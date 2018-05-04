// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The XCurrency developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

#include "addresstablemodel.h"
#include "askpassphrasedialog.h"
#include "bitcoinunits.h"
#include "clientmodel.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "walletmodel.h"

#include "base58.h"
#include "coincontrol.h"
#include "ui_interface.h"
#include "utilmoneystr.h"
#include "wallet.h"
#include "rpcclient.h"
#include "rpcserver.h"

#include "json/json_spirit.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QFileDialog>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>

using namespace json_spirit;

//******************************************************************************
//******************************************************************************
SendCoinsDialog::SendCoinsDialog(QWidget* parent) : QDialog(parent),
                                                    ui(new Ui::SendCoinsDialog),
                                                    clientModel(0),
                                                    model(0),
                                                    fNewRecipientAllowed(true),
                                                    fFeeMinimized(true)
{
    ui->setupUi(this);

    on_txWithImage_clicked();

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

    GUIUtil::setupAddressWidget(ui->lineEditCoinControlChange, this);

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString&)), this, SLOT(coinControlChangeEdited(const QString&)));

    // UTXO Splitter
    connect(ui->splitBlockCheckBox, SIGNAL(stateChanged(int)), this, SLOT(splitBlockChecked(int)));
    connect(ui->splitBlockLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(splitBlockLineEditChanged(const QString&)));

    // XCurrency specific
    QSettings settings;
    if (!settings.contains("bUseObfuScation"))
        settings.setValue("bUseObfuScation", false);
    if (!settings.contains("bUseSwiftTX"))
        settings.setValue("bUseSwiftTX", false);

    bool useObfuScation = settings.value("bUseObfuScation").toBool();

    // STG Disable obfuscation until ready...
    useObfuScation = false;
    ui->checkUseObfuscation->setVisible(false);

    bool useSwiftTX = settings.value("bUseSwiftTX").toBool();
    if (fLiteMode)
    {
        ui->checkUseObfuscation->setChecked(false);
        ui->checkUseObfuscation->setVisible(false);
        ui->checkSwiftTX->setVisible(false);
        CoinControlDialog::coinControl->useObfuScation = false;
        CoinControlDialog::coinControl->useSwiftTX = false;
    }
    else
    {
        ui->checkUseObfuscation->setChecked(useObfuScation);
        ui->checkSwiftTX->setChecked(useSwiftTX);
        CoinControlDialog::coinControl->useObfuScation = useObfuScation;
        CoinControlDialog::coinControl->useSwiftTX = useSwiftTX;
    }

    connect(ui->checkUseObfuscation, SIGNAL(stateChanged(int)), this, SLOT(updateDisplayUnit()));
    connect(ui->checkSwiftTX, SIGNAL(stateChanged(int)), this, SLOT(updateSwiftTX()));

    // Coin Control: clipboard actions
    QAction* clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction* clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction* clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction* clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction* clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction* clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction* clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction* clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    // init transaction fee section
    if (!settings.contains("fFeeSectionMinimized"))
        settings.setValue("fFeeSectionMinimized", true);
    if (!settings.contains("nFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nFeeRadio", 1);                                                                                             // custom
    if (!settings.contains("nFeeRadio"))
        settings.setValue("nFeeRadio", 0);                                                                                                   // recommended
    if (!settings.contains("nCustomFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nCustomFeeRadio", 1);                                                                                             // total at least
    if (!settings.contains("nCustomFeeRadio"))
        settings.setValue("nCustomFeeRadio", 0); // per kilobyte
    if (!settings.contains("nSmartFeeSliderPosition"))
        settings.setValue("nSmartFeeSliderPosition", 0);
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);
    if (!settings.contains("fPayOnlyMinFee"))
        settings.setValue("fPayOnlyMinFee", false);
    if (!settings.contains("fSendFreeTransactions"))
        settings.setValue("fSendFreeTransactions", false);

    ui->groupFee->setId(ui->radioSmartFee, 0);
    ui->groupFee->setId(ui->radioCustomFee, 1);
    ui->groupFee->button((int)std::max(0, std::min(1, settings.value("nFeeRadio").toInt())))->setChecked(true);
    ui->groupCustomFee->setId(ui->radioCustomPerKilobyte, 0);
    ui->groupCustomFee->setId(ui->radioCustomAtLeast, 1);
    ui->groupCustomFee->button((int)std::max(0, std::min(1, settings.value("nCustomFeeRadio").toInt())))->setChecked(true);
    ui->sliderSmartFee->setValue(settings.value("nSmartFeeSliderPosition").toInt());
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
    ui->checkBoxMinimumFee->setChecked(settings.value("fPayOnlyMinFee").toBool());
    ui->checkBoxFreeTx->setChecked(settings.value("fSendFreeTransactions").toBool());
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::setClientModel(ClientModel* clientModel)
{
    this->clientModel = clientModel;

    if (clientModel)
    {
        connect(clientModel, SIGNAL(numBlocksChanged(int)),
                this,        SLOT(updateSmartFeeLabel()));
    }
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::setModel(WalletModel* model)
{
    this->model = model;

    if (model && model->getOptionsModel())
    {
        for (int i = 0; i < ui->entries->count(); ++i)
        {
            SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if (entry)
            {
                entry->setModel(model);
            }
        }

        setBalance(model->getBalance(),
                   model->getUnconfirmedBalance(),
                   model->getImmatureBalance(),
                   model->getAnonymizedBalance(),
                   model->getWatchBalance(),
                   model->getWatchUnconfirmedBalance(),
                   model->getWatchImmatureBalance());

        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)),
                this,  SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)),
                this,                     SLOT(updateDisplayUnit()));

        updateDisplayUnit();

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
        coinControlUpdateLabels();

        // fee section
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupCustomFee, SIGNAL(buttonClicked(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->groupCustomFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(coinControlUpdateLabels()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->checkBoxFreeTx, SIGNAL(stateChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->checkBoxFreeTx, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
        ui->customFee->setSingleStep(CWallet::minTxFee.GetFeePerK());
        updateFeeSectionControls();
        updateMinFeeLabel();
        updateSmartFeeLabel();
        updateGlobalFeeVariables();
    }
}

//******************************************************************************
//******************************************************************************
SendCoinsDialog::~SendCoinsDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    settings.setValue("nFeeRadio", ui->groupFee->checkedId());
    settings.setValue("nCustomFeeRadio", ui->groupCustomFee->checkedId());
    settings.setValue("nSmartFeeSliderPosition", ui->sliderSmartFee->value());
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());
    settings.setValue("fPayOnlyMinFee", ui->checkBoxMinimumFee->isChecked());
    settings.setValue("fSendFreeTransactions", ui->checkBoxFreeTx->isChecked());

    delete ui;
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::on_sendButton_clicked()
{
    if (!model || !model->getOptionsModel())
    {
        return;
    }

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for (int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());

        // UTXO splitter - address should be our own
        CBitcoinAddress address = entry->getValue().address.toStdString();
        if (!model->isMine(address) && ui->splitBlockCheckBox->checkState() == Qt::Checked)
        {
            CoinControlDialog::coinControl->fSplitBlock = false;
            ui->splitBlockCheckBox->setCheckState(Qt::Unchecked);
            QMessageBox::warning(this,
                                 tr("Send Coins"),
                                 tr("The split block tool does not work when sending to outside addresses. Try again."),
                                 QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        if (entry)
        {
            if (entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    // data item
    if (ui->txWithImage->isChecked())
    {
        QByteArray data;
        if (!loadDataFile(data) || data.size() == 0)
        {
            return;
        }

        recipients.append(SendCoinsRecipient(data));
    }

    if (!valid || recipients.isEmpty())
    {
        return;
    }

    // set split block in model
    CoinControlDialog::coinControl->fSplitBlock = ui->splitBlockCheckBox->checkState() == Qt::Checked;

    if (ui->entries->count() > 1 && ui->splitBlockCheckBox->checkState() == Qt::Checked)
    {
        CoinControlDialog::coinControl->fSplitBlock = false;
        ui->splitBlockCheckBox->setCheckState(Qt::Unchecked);
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The split block tool does not work with multiple addresses. Try again."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    if (CoinControlDialog::coinControl->fSplitBlock)
    {
        CoinControlDialog::coinControl->nSplitBlock = int(ui->splitBlockLineEdit->text().toInt());
    }

    QString strFunds = tr("using") + " <b>" + tr("anonymous funds") + "</b>";
    QString strFee = "";
    recipients[0].inputType = ONLY_DENOMINATED;

    if (ui->checkUseObfuscation->isChecked())
    {
        recipients[0].inputType = ONLY_DENOMINATED;
        strFunds = tr("using") + " <b>" + tr("anonymous funds") + "</b>";
        QString strNearestAmount(
            BitcoinUnits::formatWithUnit(
                model->getOptionsModel()->getDisplayUnit(), 0.1 * COIN));
        strFee = QString(tr(
            "(obfuscation requires this amount to be rounded up to the nearest %1).")
                             .arg(strNearestAmount));
    }
    else
    {
        recipients[0].inputType = ALL_COINS;
        strFunds = tr("using") + " <b>" + tr("any available funds (not recommended)") + "</b>";
    }

    if (ui->checkSwiftTX->isChecked())
    {
        recipients[0].useSwiftTX = true;
        strFunds += " ";
        strFunds += tr("and SwiftTX");
    }
    else
    {
        recipients[0].useSwiftTX = false;
    }


    // Format confirmation message
    QStringList formatted;
    foreach (const SendCoinsRecipient& rcp, recipients)
    {
        // generate bold amount string
        QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount);
        amount.append("</b> ").append(strFunds);

        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        if (!rcp.paymentRequest.IsInitialized())
        {
            // normal payment
            if (rcp.label.length() > 0)
            {
                // label with address
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else
            {
                // just address
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if (!rcp.authenticatedMerchant.isEmpty())
        {
            // secure payment request
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else
        {
            // insecure payment request
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        if (fSplitBlock)
        {
            recipientElement.append(tr(" split into %1 outputs using the UTXO splitter.").arg(CoinControlDialog::coinControl->nSplitBlock));
        }

        formatted.append(recipientElement);
    }

    fNewRecipientAllowed = false;

    // request unlock only if was locked or unlocked for mixing:
    // this way we let users unlock by walletpassphrase or by menu
    // and make many transactions while unlocking through this dialog
    // will call relock
    WalletModel::EncryptionStatus encStatus = model->getEncryptionStatus();
    if (encStatus == model->Locked || encStatus == model->UnlockedForAnonymizationOnly)
    {
        WalletModel::UnlockContext ctx(model->requestUnlock(true));
        if (!ctx.isValid())
        {
            // Unlock wallet was cancelled
            fNewRecipientAllowed = true;
            return;
        }
        send(recipients, strFee, formatted);
        return;
    }
    // already unlocked or not encrypted at all
    send(recipients, strFee, formatted);
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::send(QList<SendCoinsRecipient> recipients, QString strFee, QStringList formatted)
{
    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    if (model->getOptionsModel()->getCoinControlFeatures())
    {
        // coin control enabled
        prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
    }
    else
    {
        prepareStatus = model->prepareTransaction(currentTransaction);
    }

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()), true);

    if (prepareStatus.status != WalletModel::OK)
    {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();
    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if (txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("are added as transaction fee"));
        questionString.append(" ");
        questionString.append(strFee);

        // append transaction size
        questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    foreach (BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
    {
        if (u != model->getOptionsModel()->getDisplayUnit())
        {
            alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(u, totalAmount));
        }
    }

    // Show total amount + all alternative units
    questionString.append(tr("Total Amount = <b>%1</b><br />= %2")
                              .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount))
                              .arg(alternativeUnits.join("<br />= ")));

    // Limit number of displayed entries
    int messageEntries = formatted.size();
    int displayedEntries = 0;
    for (int i = 0; i < formatted.size(); i++)
    {
        if (i >= MAX_SEND_POPUP_ENTRIES)
        {
            formatted.removeLast();
            i--;
        }
        else
        {
            displayedEntries = i + 1;
        }
    }
    questionString.append("<hr />");
    questionString.append(tr("<b>(%1 of %2 entries displayed)</b>").arg(displayedEntries).arg(messageEntries));

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"),
        questionString.arg(formatted.join("<br />")),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while (ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::reject()
{
    clear();
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::accept()
{
    clear();
}

//******************************************************************************
//******************************************************************************
SendCoinsEntry* SendCoinsDialog::addEntry()
{
    SendCoinsEntry* entry = new SendCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    updateTabsAndLabels();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if (bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

//******************************************************************************
//******************************************************************************
QWidget* SendCoinsDialog::setupTabChain(QWidget* prev)
{
    for (int i = 0; i < ui->entries->count(); ++i) {
        SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if (entry) {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::setAddress(const QString& address)
{
    SendCoinsEntry* entry = 0;
    // Replace the first entry if it is still unused
    if (ui->entries->count() == 1) {
        SendCoinsEntry* first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if (first->isClear()) {
            entry = first;
        }
    }
    if (!entry) {
        entry = addEntry();
    }

    entry->setAddress(address);
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::pasteEntry(const SendCoinsRecipient& rv)
{
    if (!fNewRecipientAllowed)
        return;

    SendCoinsEntry* entry = 0;
    // Replace the first entry if it is still unused
    if (ui->entries->count() == 1) {
        SendCoinsEntry* first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if (first->isClear()) {
            entry = first;
        }
    }
    if (!entry) {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

//******************************************************************************
//******************************************************************************
bool SendCoinsDialog::handlePaymentRequest(const SendCoinsRecipient& rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(anonymizedBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    if (model && model->getOptionsModel()) {
        uint64_t bal = 0;
        QSettings settings;
        settings.setValue("bUseObfuScation", ui->checkUseObfuscation->isChecked());
        if (ui->checkUseObfuscation->isChecked()) {
            bal = anonymizedBalance;
        } else {
            bal = balance;
        }

        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), bal));
    }
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateDisplayUnit()
{
    TRY_LOCK(cs_main, lockMain);
    if (!lockMain) return;

    setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(),
        model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
    CoinControlDialog::coinControl->useObfuScation = ui->checkUseObfuscation->isChecked();
    coinControlUpdateLabels();
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateSwiftTX()
{
    QSettings settings;
    settings.setValue("bUseSwiftTX", ui->checkSwiftTX->isChecked());
    CoinControlDialog::coinControl->useSwiftTX = ui->checkSwiftTX->isChecked();
    coinControlUpdateLabels();
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn& sendCoinsReturn, const QString& msgArg, bool fPrepare)
{
    bool fAskForUnlock = false;
    
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch (sendCoinsReturn.status) {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid, please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found, can only send to each address once per send operation.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AnonymizeOnlyUnlocked:
        // Unlock is only need when the coins are send
        if(!fPrepare)
            fAskForUnlock = true;
        else
            msgParams.first = tr("Error: The wallet was unlocked only to anonymize coins.");
        break;

    case WalletModel::InsaneFee:
        msgParams.first = tr("A fee %1 times higher than %2 per kB is considered an insanely high fee.").arg(10000).arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ::minRelayTxFee.GetFeePerK()));
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    // Unlock wallet if it wasn't fully unlocked already
    if(fAskForUnlock) {
        model->requestUnlock(false);
        if(model->getEncryptionStatus () != WalletModel::Unlocked) {
            msgParams.first = tr("Error: The wallet was unlocked only to anonymize coins. Unlock canceled.");
        }
        else {
            // Wallet unlocked
            return;
        }
    }

    emit message(tr("Send Coins"), msgParams.first, msgParams.second);
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::setMinimumFee()
{
    ui->radioCustomPerKilobyte->setChecked(true);
    ui->customFee->setValue(CWallet::minTxFee.GetFeePerK());
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateFeeSectionControls()
{
    ui->sliderSmartFee->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee2->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee3->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelFeeEstimation->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFeeNormal->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFeeFast->setEnabled(ui->radioSmartFee->isChecked());
    ui->checkBoxMinimumFee->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelMinFeeWarning->setEnabled(ui->radioCustomFee->isChecked());
    ui->radioCustomPerKilobyte->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->radioCustomAtLeast->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->customFee->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateGlobalFeeVariables()
{
    if (ui->radioSmartFee->isChecked()) {
        nTxConfirmTarget = (int)25 - (int)std::max(0, std::min(24, ui->sliderSmartFee->value()));
        payTxFee = CFeeRate(0);
    } else {
        nTxConfirmTarget = 25;
        payTxFee = CFeeRate(ui->customFee->value());
        fPayAtLeastCustomFee = ui->radioCustomAtLeast->isChecked();
    }

    fSendFreeTransactions = ui->checkBoxFreeTx->isChecked();
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateFeeMinimizedLabel()
{
    if (!model || !model->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) +
                                       ((ui->radioCustomPerKilobyte->isChecked()) ? "/kB" : ""));
    }
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateMinFeeLabel()
{
    if (model && model->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the minimum fee of %1").arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), CWallet::minTxFee.GetFeePerK()) + "/kB"));
}

//******************************************************************************
//******************************************************************************
void SendCoinsDialog::updateSmartFeeLabel()
{
    if (!model || !model->getOptionsModel())
        return;

    int nBlocksToConfirm = (int)25 - (int)std::max(0, std::min(24, ui->sliderSmartFee->value()));
    CFeeRate feeRate = mempool.estimateFee(nBlocksToConfirm);
    if (feeRate <= CFeeRate(0)) // not enough data => minfee
    {
        ui->labelSmartFee->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), CWallet::minTxFee.GetFeePerK()) + "/kB");
        ui->labelSmartFee2->show(); // (Smart fee not initialized yet. This usually takes a few blocks...)
        ui->labelFeeEstimation->setText("");
    } else {
        ui->labelSmartFee->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), feeRate.GetFeePerK()) + "/kB");
        ui->labelSmartFee2->hide();
        ui->labelFeeEstimation->setText(tr("Estimated to begin confirmation within %n block(s).", "", nBlocksToConfirm));
    }

    updateFeeMinimizedLabel();
}

//******************************************************************************
// UTXO splitter
//******************************************************************************
void SendCoinsDialog::splitBlockChecked(int state)
{
    if (model) {
        CoinControlDialog::coinControl->fSplitBlock = (state == Qt::Checked);
        fSplitBlock = (state == Qt::Checked);
        ui->splitBlockLineEdit->setEnabled((state == Qt::Checked));
        ui->labelBlockSizeText->setEnabled((state == Qt::Checked));
        ui->labelBlockSize->setEnabled((state == Qt::Checked));
        coinControlUpdateLabels();
    }
}

//******************************************************************************
//UTXO splitter
//******************************************************************************
void SendCoinsDialog::splitBlockLineEditChanged(const QString& text)
{
    //grab the amount in Coin Control AFter Fee field
    QString qAfterFee = ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace("~", "").simplified().replace(" ", "");

    //convert to CAmount
    CAmount nAfterFee;
    ParseMoney(qAfterFee.toStdString().c_str(), nAfterFee);

    //if greater than 0 then divide after fee by the amount of blocks
    CAmount nSize = nAfterFee;
    int nBlocks = text.toInt();
    if (nAfterFee && nBlocks)
        nSize = nAfterFee / nBlocks;

    //assign to split block dummy, which is used to recalculate the fee amount more outputs
    CoinControlDialog::nSplitBlockDummy = nBlocks;

    //update labels
    ui->labelBlockSize->setText(QString::fromStdString(FormatMoney(nSize)));
    coinControlUpdateLabels();
}

//******************************************************************************
// Coin Control: copy label "Quantity" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

//******************************************************************************
// Coin Control: copy label "Amount" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

//******************************************************************************
// Coin Control: copy label "Fee" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")).replace("~", ""));
}

//******************************************************************************
// Coin Control: copy label "After fee" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace("~", ""));
}

//******************************************************************************
// Coin Control: copy label "Bytes" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text().replace("~", ""));
}

//******************************************************************************
// Coin Control: copy label "Priority" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardPriority()
{
    GUIUtil::setClipboard(ui->labelCoinControlPriority->text());
}

//******************************************************************************
// Coin Control: copy label "Dust" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

//******************************************************************************
// Coin Control: copy label "Change" to clipboard
//******************************************************************************
void SendCoinsDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")).replace("~", ""));
}

//******************************************************************************
// Coin Control: settings menu - coin control enabled/disabled by user
//******************************************************************************
void SendCoinsDialog::coinControlFeatureChanged(bool checked)
{
    ui->frameCoinControl->setVisible(checked);

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();

    if (checked)
        coinControlUpdateLabels();
}

//******************************************************************************
// Coin Control: button inputs -> show actual coin control dialog
//******************************************************************************
void SendCoinsDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg;
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

//******************************************************************************
// Coin Control: checkbox custom change address
//******************************************************************************
void SendCoinsDialog::coinControlChangeChecked(int state)
{
    if (state == Qt::Unchecked) {
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->clear();
    } else
        // use this to re-validate an already entered address
        coinControlChangeEdited(ui->lineEditCoinControlChange->text());

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
}

//******************************************************************************
// Coin Control: custom change address changed
//******************************************************************************
void SendCoinsDialog::coinControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel()) {
        // Default to no change address until verified
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        CBitcoinAddress addr = CBitcoinAddress(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelCoinControlChangeLabel->setText("");
        } else if (!addr.IsValid()) // Invalid address
        {
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid XCurrency address"));
        } else // Valid address
        {
            CPubKey pubkey;
            CKeyID keyid;
            addr.GetKeyID(keyid);
            if (!model->getPubKey(keyid, pubkey)) // Unknown change address
            {
                ui->labelCoinControlChangeLabel->setText(tr("Warning: Unknown change address"));
            } else // Known change address
            {
                ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                if (!associatedLabel.isEmpty())
                    ui->labelCoinControlChangeLabel->setText(associatedLabel);
                else
                    ui->labelCoinControlChangeLabel->setText(tr("(no label)"));

                CoinControlDialog::coinControl->destChange = addr.Get();
            }
        }
    }
}

//******************************************************************************
// Coin Control: update labels
//******************************************************************************
void SendCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
        return;

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    for (int i = 0; i < ui->entries->count(); ++i) {
        SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if (entry)
            CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }

    ui->checkUseObfuscation->setChecked(CoinControlDialog::coinControl->useObfuScation);

    if (CoinControlDialog::coinControl->HasSelected()) {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    } else {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::on_txWithImage_clicked()
{
    bool checked = (ui->txWithImage->checkState() == Qt::Checked);
    ui->imagePath->setEnabled(checked);
    ui->selectImageButton->setEnabled(checked);
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::on_selectImageButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    trUtf8("Select file"),
                                                    trUtf8("Select file for send"),
                                                    trUtf8("Document files (*.pdf)"));
    if (fileName.isEmpty())
    {
        return;
    }

    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

    if(!mimeType.inherits(QLatin1String("application/pdf")))
    {
        QMessageBox::warning(this, tr("File type"),
                             tr("File doesn't match PDF type."),
                             QMessageBox::Ok);
        return;
    }
    ui->imagePath->setText(fileName);
}

//*****************************************************************************
//*****************************************************************************
bool SendCoinsDialog::loadDataFile(QByteArray & data)
{
    QFileInfo inf(ui->imagePath->text());

    // add image
    if (!inf.exists())
    {
        // throw std::runtime_error("File not found");
        return false;
    }
    if (inf.size() > 1024*1024)
    {
        // size must be less than 1M
        // throw std::runtime_error("Image file size too big");
        return false;
    }

    QFile f(inf.absoluteFilePath());
    if (!f.open(QFile::ReadOnly))
    {
        // throw std::runtime_error("File not opened");
        return false;
    }

    data = f.readAll();

    return true;
}

//*****************************************************************************
//*****************************************************************************
void SendCoinsDialog::processPaymentsWithImage()
{
//    const static std::string createCommand("createrawtransaction");
//    const static std::string signCommand("signrawtransaction");
//    const static std::string sendCommand("sendrawtransaction");

//    int errCode = 0;
//    std::string errMessage;

//    try
//    {
////        SendCoinsRecipient rcp = m_sendEntries.front()->getValue();

//        std::vector<COutput> vCoins;
//        pwalletMain->AvailableCoins(vCoins, true, nullptr);

//        int64_t amount = 0;
//        std::vector<COutput> used;

//        for (const COutput & out : vCoins)
//        {
//            amount += out.tx->vout[out.i].nValue;
//            used.push_back(out);

//            if (amount >= rcp.amount + nTransactionFee)
//            {
//                break;
//            }
//        }

//        int64_t nPayFee = (nTransactionFee == 0 ? MIN_TX_FEE : nTransactionFee) * (1 + (uint64_t)inf.size() / 1000);
//        if (amount < rcp.amount + nPayFee)
//        {
//            throw std::runtime_error("No money");
//        }

//        Array inputs;
//        for (const COutput & out : used)
//        {
//            Object tmp;
//            tmp.push_back(Pair("txid", out.tx->GetHash().ToString()));
//            tmp.push_back(Pair("vout", out.i));
//            inputs.push_back(tmp);
//        }

//        QFileInfo inf(ui->imagePath->text());

//        Object outputs;
//        {
//            // add image
//            if (!inf.exists())
//            {
//                throw std::runtime_error("File not found");
//            }
//            if (inf.size() > 1024*1024)
//            {
//                // size must be less than 1M
//                throw std::runtime_error("Image file size too big");
//            }

//            {
//                QFile f(inf.absoluteFilePath());
//                if (!f.open(QFile::ReadOnly))
//                {
//                    throw std::runtime_error("File not opened");
//                }

//                QByteArray data = f.readAll();

//                std::string strdata = HexStr(data.begin(), data.end());

//               outputs.push_back(Pair("data", strdata));
//            }
//        }
//        outputs.push_back(Pair(rcp.address.toStdString(), (double)rcp.amount/COIN));

//        Value result;

//        {
//            Array params;
//            params.push_back(inputs);
//            params.push_back(outputs);

//            // call create
//            result = tableRPC.execute(createCommand, params);
//            if (result.type() != str_type)
//            {
//                throw std::runtime_error("Create transaction failed");
//            }
//        }

//        {
//            std::vector<std::string> params;

//            std::string rawtx = result.get_str();
//            params.push_back(rawtx);

//            result = tableRPC.execute(signCommand, RPCConvertValues(signCommand, params));
//            if (result.type() != str_type)
//            {
//                throw std::runtime_error("Sign transaction failed");
//            }
//        }

//        {
//            std::string rawtx = result.get_str();
//            std::vector<std::string> params;
//            params.push_back(rawtx);

//            result = tableRPC.execute(sendCommand, RPCConvertValues(sendCommand, params));
//        }
//    }
//    catch (json_spirit::Object & obj)
//    {
//        //
//        errCode = find_value(obj, "code").get_int();
//        errMessage = find_value(obj, "message").get_str();
//    }
//    catch (std::runtime_error & e)
//    {
//        // specified error
//        errCode = -1;
//        errMessage = e.what();
//    }
//    catch (...)
//    {
//        errCode = -1;
//        errMessage = "unknown error";
//    }

//    if (errCode != 0)
//    {
//        QMessageBox::warning(this, trUtf8("Send Coins"),
//            trUtf8("Failed, code %1\n%2").arg(QString::number(errCode), QString::fromStdString(errMessage)),
//            QMessageBox::Ok, QMessageBox::Ok);
//    }
}
