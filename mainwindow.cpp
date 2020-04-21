
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCore/QtCore>
#include <iostream>
#include <sstream>



#include "LiveAmp.h"

#define LIBVERSIONSTREAM(version) version.Major << "." << version.Minor << "." << version.Build << "." << version.Revision
#define LSLVERSIONSTREAM(version) (version/100) << "." << (version%100)
#define APPVERSIONSTREAM(version) version.Major << "." << version.Minor << "." << version.Bugfix

const int pnSamplingRates[] = {250,500,1000};
int getSamplingRateIndex(int nSamplingRate)
{
	switch (nSamplingRate)
	{
	case (250):
		return 0;
	case (500):
		return 1;
	case(1000):
		return 2;
	default:
		return 0;
	}
}

MainWindow::MainWindow(QWidget *parent, const char* config_file): QMainWindow(parent),ui(new Ui::MainWindow) 
{
	
	m_AppVersion.Major = 1;
	m_AppVersion.Minor = 19;
	m_AppVersion.Bugfix = 0;

	m_bOverrideAutoUpdate = false;
	ui->setupUi(this);
	LoadConfig(config_file);

	QObject::connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(Close()));
	QObject::connect(ui->linkButton, SIGNAL(clicked()), this, SLOT(Link()));
	QObject::connect(ui->actionLoad_Configuration, SIGNAL(triggered()), this, SLOT(LoadConfigDialog()));
	QObject::connect(ui->actionSave_Configuration, SIGNAL(triggered()), this, SLOT(SaveConfigDialog()));
	QObject::connect(ui->actionVersions, SIGNAL(triggered()), this, SLOT(VersionsCialog()));
	QObject::connect(ui->refreshDevices,SIGNAL(clicked()),this,SLOT(RefreshDevices()));
	QObject::connect(ui->eegChannelCount, SIGNAL(valueChanged(int)),this, SLOT(UpdateChannelLabelsWithEeg(int)));
	QObject::connect(ui->bipolarChannelCount, SIGNAL(valueChanged(int)),this, SLOT(UpdateChannelLabelsWithBipolar(int)));
	QObject::connect(ui->deviceCb,SIGNAL(currentIndexChanged(int)),this,SLOT(ChooseDevice(int)));
	QObject::connect(ui->auxChannelCount, SIGNAL(valueChanged(int)), this, SLOT(UpdateChannelLabelsAux(int)));
	QObject::connect(ui->useACC, SIGNAL(clicked(bool)), this, SLOT(UpdateChannelLabelsAcc(bool)));
	QObject::connect(ui->sampleCounter, SIGNAL(clicked(bool)), this, SLOT(UpdateChannelLabelsSampleCounter(bool)));
	QObject::connect(ui->rbSync, SIGNAL(clicked(bool)), this, SLOT(RadioButtonBehavior(bool)));
	QObject::connect(ui->rbMirror, SIGNAL(clicked(bool)), this, SLOT(RadioButtonBehavior(bool)));
}

void MainWindow::UpdateChannelCounters(int n)
{

	ui->eegChannelCount->setMaximum(n);
	ui->eegChannelCount->setValue(n-ui->bipolarChannelCount->value());
	UpdateChannelLabelsWithBipolar(ui->bipolarChannelCount->value());

}

void MainWindow::UpdateChannelLabels(void) 
{

	if (!ui->overwriteChannelLabels->isChecked())return;

	int nEeg = ui->eegChannelCount->value();
	int nBip = ui->bipolarChannelCount->value();
	int nAux = ui->auxChannelCount->value();
	int nAcc = (ui->useACC->isChecked()) ? 3 : 0;
	bool bUseSampleCounter = ui->sampleCounter->isChecked();
	int nMaxEEG = nEeg > 32 ? 64 : 32;
	m_bOverrideAutoUpdate = true;
	// TODO: parameterize this to follow behavior according to 
	// available channels
	if (nEeg > nMaxEEG - 8 && nBip != 0) {
		ui->eegChannelCount->setMaximum(nMaxEEG - 8);
		nEeg = nMaxEEG - 8;
	}
	else
		ui->eegChannelCount->setMaximum(nMaxEEG);
	m_bOverrideAutoUpdate = false;

	int nTotalElectrodes = nEeg + nBip;
	std::string str;
	std::vector<std::string> psEEGChannelLabels;
	std::istringstream iss(ui->channelLabels->toPlainText().toStdString()); 
	while (std::getline(iss, str, '\n'))
		psEEGChannelLabels.push_back(str);
	//str = ui->channelLabels->toPlainText().toStdString();
	//boost::split(psEEGChannelLabels, str, boost::is_any_of("\n"));
	int nVal = ui->eegChannelCount->value();

	while (int i = psEEGChannelLabels.size() > nVal)
		psEEGChannelLabels.pop_back();
	if(psEEGChannelLabels.size()>0)
		while (psEEGChannelLabels[psEEGChannelLabels.size() - 1].find("*") != std::string::npos)
			psEEGChannelLabels.pop_back();

	ui->channelLabels->clear();
	for (int i = 1; i <= nTotalElectrodes; i++) {
		if (i - 1 < psEEGChannelLabels.size())
			str = psEEGChannelLabels[i - 1];
		else
			str = std::to_string(i);
		if (str.compare("ACC_X") == 0)
			str = std::to_string(i);
		if (str.compare("ACC_Y") == 0)
			str = std::to_string(i);
		if (str.compare("ACC_Z") == 0)
			str = std::to_string(i);
		if (i>ui->eegChannelCount->value())str += "*";
		ui->channelLabels->appendPlainText(str.c_str());
	}


	for (int i = 1; i <= nAux; i++) 
	{
		str = "AUX_" + std::to_string(i);
		ui->channelLabels->appendPlainText(str.c_str());
	}

	if (nAcc == 3) 
		ui->channelLabels->appendPlainText("ACC_X\nACC_Y\nACC_Z");

	//if (bUseSampleCounter)
	//{
	//	str = "SampleCounter";
	//	ui->channelLabels->appendPlainText(str.c_str());
	//}
}

void MainWindow::UpdateChannelLabelsWithEeg(int n){
	
	if (m_bOverrideAutoUpdate)return;
	UpdateChannelLabels();
}

void MainWindow::UpdateChannelLabelsWithBipolar(int n){
	
	if(!ui->overwriteChannelLabels->isChecked())return;
	UpdateChannelLabels();
	
}

void MainWindow::LoadConfigDialog() 
{
	QString filename = QFileDialog::getOpenFileName(this,"Load Configuration File","","Configuration Files (*.cfg)");
	if (!filename.isEmpty())
		LoadConfig(filename);
}

void MainWindow::SaveConfigDialog() 
{
	QString filename = QFileDialog::getSaveFileName(this,"Save Configuration File","","Configuration Files (*.cfg)");
	if (!filename.isEmpty())
		SaveConfig(filename);
}

void MainWindow::VersionsDialog()
{
	t_VersionNumber libVersion;
	GetLibraryVersion(&libVersion);
	int32_t lslProtocolVersion = lsl::protocol_version();
	int32_t lslLibVersion = lsl::library_version();
	std::stringstream ss;
	ss << "Amplifier_LIB: " << LIBVERSIONSTREAM(libVersion)  << "\n" <<
		  "lsl protocol: " << LSLVERSIONSTREAM(lslProtocolVersion) << "\n" <<
		  "liblsl: " << LSLVERSIONSTREAM(lslLibVersion) << "\n" <<
		  "App: " << APPVERSIONSTREAM(m_AppVersion) << "_beta";
	QMessageBox::information(this, "Versions", ss.str().c_str(), QMessageBox::Ok);
}

void MainWindow::UpdateChannelLabelsAux(int n) 
{
	UpdateChannelLabels();
}

void MainWindow::UpdateChannelLabelsAcc(bool b) 
{
	UpdateChannelLabels();
}

void MainWindow::UpdateChannelLabelsSampleCounter(bool b)
{
	//UpdateChannelLabels();
}

void MainWindow::CloseEvent(QCloseEvent *ev) 
{
	if (m_ptReaderThread)
		ev->ignore();
}

void MainWindow::LoadConfig(const QString& filename) 
{
	QSettings pt(filename, QSettings::IniFormat);

	ui->deviceSerialNumber->setText(pt.value("settings/deviceSerialNumber", "x-0077").toString());
	ui->eegChannelCount->setValue(pt.value("settings/channelcount", 32).toInt());
	m_nEegChannelCount = ui->eegChannelCount->value();
	ui->bipolarChannelCount->setValue(pt.value("settings/bipolarcount", 0).toInt());
	ui->chunkSize->setValue(pt.value("settings/chunksize", 10).toInt());
	int idx = getSamplingRateIndex(pt.value("settings/samplingrate", 250).toInt());
	ui->samplingRate->setCurrentIndex(idx);
	ui->auxChannelCount->setValue(pt.value("settings/auxChannelCount", 0).toInt());
	ui->useACC->setCheckState(pt.value("settings/useACC", true).toBool() ? Qt::Checked : Qt::Unchecked);
	ui->unsampledMarkers->setCheckState(pt.value("settings/unsampledmarkers", true).toBool() ? Qt::Checked : Qt::Unchecked);
	ui->sampledMarkersEEG->setCheckState(pt.value("settings/sampledmarkersEEG", false).toBool() ? Qt::Checked : Qt::Unchecked);
	ui->overwriteChannelLabels->setCheckState(pt.value("settings/overwrite", true).toBool() ? Qt::Checked : Qt::Unchecked);
	t_TriggerOutputMode triggerOutputMode = (t_TriggerOutputMode)pt.value("settings/triggerOutputMode", 0).toInt();
	if (triggerOutputMode == TM_SYNC)ui->rbSync->setChecked(true);
	else if (triggerOutputMode == TM_MIRROR)ui->rbMirror->setChecked(true);
	else ui->rbDefault->setChecked(true);
	RadioButtonBehavior(true);
	int syncFreq = (pt.value("settings/syncFrequency", 1).toInt());
	ui->sbSyncFreq->setValue((syncFreq < 1) ? 1 : syncFreq);
	ui->channelLabels->clear();
	ui->channelLabels->setPlainText(pt.value("channels/labels").toStringList().join('\n'));
	UpdateChannelLabels();

}

void MainWindow::SaveConfig(const QString& filename) 
{
	QSettings pt(filename, QSettings::IniFormat);
	pt.beginGroup("settings");
	pt.setValue("deviceSerialNumber", ui->deviceSerialNumber->text());
	pt.setValue("channelcount", ui->eegChannelCount->value());
	pt.setValue("bipolarcount", ui->bipolarChannelCount->value());
	pt.setValue("chunksize", ui->chunkSize->value());
	pt.setValue("samplingrate", ui->samplingRate->currentText());
	pt.setValue("auxChannelCount", ui->auxChannelCount->value());
	pt.setValue("useACC", ui->useACC->checkState() == Qt::Checked);
	pt.setValue("unsampledmarkers", ui->unsampledMarkers->checkState() == Qt::Checked);
	pt.setValue("sampledmarkersEEG", ui->sampledMarkersEEG->checkState() == Qt::Checked);
	pt.setValue("overwrite", ui->overwriteChannelLabels->checkState() == Qt::Checked);
	pt.setValue("triggerOutputMode", m_TriggerOutputMode);
	pt.setValue("syncFrequency", ui->sbSyncFreq->value());
	pt.endGroup();

	pt.beginGroup("channels");
	pt.setValue("labels", ui->channelLabels->toPlainText().split('\n'));
	pt.endGroup();
}

void MainWindow::RadioButtonBehavior(bool b) 
{
	if (ui->rbSync->isChecked())
	{
		ui->sbSyncFreq->setEnabled(true);
		m_TriggerOutputMode = TM_SYNC;
	}
	if (ui->rbMirror->isChecked())
	{
		ui->sbSyncFreq->setEnabled(false);
		m_TriggerOutputMode = TM_MIRROR;
	}
	if (ui->rbDefault->isChecked()) 
	{
		ui->sbSyncFreq->setEnabled(false);
		m_TriggerOutputMode = TM_DEFAULT;
	}
}

void MainWindow::WaitMessage()
{

	QMessageBox msgBox;
	msgBox.setWindowTitle("Please Wait");
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText("connecting to devices");
	msgBox.addButton(QMessageBox::Ok);
	msgBox.exec();
}


void MainWindow::RefreshDevices()
{
	ui->deviceCb->blockSignals(true);
	QStringList qsl;
	std::vector<std::pair<std::string, int>> ampData;
	this->setCursor(Qt::WaitCursor);
	this->setWindowTitle("Searching for Devices...");
	//wait_message();
	//ampData.clear();
	try
	{
		m_LiveAmp.enumerate(ampData, ui->useSim->checkState());
	}
	catch(std::exception &e) 
	{
		QMessageBox::critical(this,"Error",(std::string("Could not locate LiveAmp device(s): ")+=e.what()).c_str(),QMessageBox::Ok);
	}

	this->setCursor(Qt::ArrowCursor);

	if(!m_psLiveAmpSns.empty())m_psLiveAmpSns.clear();
	if(!m_pnUsableChannelsByDevice.empty()) m_pnUsableChannelsByDevice.clear();

	int foo = ui->deviceCb->count();
	if(!ampData.empty()) {
		ui->deviceCb->clear();
		std::stringstream ss;
		int i=0;
		
		for(std::vector<std::pair<std::string, int>>::iterator it=ampData.begin(); it!=ampData.end();++it){
			ss.clear();
			ss << it->first << " (" << it->second << ")";
			std::cout<<it->first<<std::endl;
			auto x = ss.str(); // oh, c++...
			qsl << QString(ss.str().c_str()); // oh, Qt...
			m_psLiveAmpSns.push_back(it->first);
			m_pnUsableChannelsByDevice.push_back(it->second);
		}
		ui->deviceCb->addItems(qsl);
		ChooseDevice(0);
	}
	
	this->setWindowTitle("LiveAmp Connector");
	ui->deviceCb->blockSignals(true);
	
}


// handle changes in chosen device
void MainWindow::ChooseDevice(int which)
{
	
	if(!m_psLiveAmpSns.empty())
		ui->deviceSerialNumber->setText(QString(m_psLiveAmpSns[which].c_str()));
	m_nEegChannelCount = m_pnUsableChannelsByDevice[ui->deviceCb->currentIndex()];
	UpdateChannelCounters(m_nEegChannelCount);

}

// TODO: make this meaningful
bool MainWindow::CheckConfiguration()
{
	bool bRes = true;
	return bRes;
}

void MainWindow::Link() 
{
	if (m_ptReaderThread) 
	{
		try 
		{
			m_bStop = true;
			m_ptReaderThread->join();
			m_ptReaderThread.reset();
			int res = SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		} 
		catch(std::exception &e) 
		{
			QMessageBox::critical(this,"Error",(std::string("Could not stop the background processing: ")+=e.what()).c_str(),QMessageBox::Ok);
			return;
		}
		this->setWindowTitle("LiveAmp Connector");
		ui->channelLabels->setEnabled(true);
		ui->overwriteChannelLabels->setEnabled(true);
		ui->STESettings->setEnabled(true);
		ui->allDeviceSettings->setEnabled(true);
		ui->linkButton->setText("Link");
	}
	else 
	{
		try 
		{
			t_AmpConfiguration ampConfiguration;
			ampConfiguration.m_sSerialNumber = ui->deviceSerialNumber->text().toStdString();
			ampConfiguration.m_nEEGChannelCount = ui->eegChannelCount->value();
			ampConfiguration.m_nBipolarChannelCount = ui->bipolarChannelCount->value();
			ampConfiguration.m_nAuxChannelCount = ui->auxChannelCount->value();
			ampConfiguration.m_nChunkSize = ui->chunkSize->value();
			ampConfiguration.m_dSamplingRate = (double)pnSamplingRates[ui->samplingRate->currentIndex()];
			ampConfiguration.m_bUseACC = ui->useACC->checkState() == Qt::Checked;
			ampConfiguration.m_bUseSampleCounter = ui->sampleCounter->checkState() == Qt::Checked;
			ampConfiguration.m_bUseSim = ui->useSim->checkState() == Qt::Checked;
			ampConfiguration.m_bUnsampledMarkers = ui->unsampledMarkers->checkState() == Qt::Checked;
			ampConfiguration.m_bSampledMarkersEEG = ui->sampledMarkersEEG->checkState()==Qt::Checked;
			ampConfiguration.m_bIsSTEInDefault = (ui->rbDefault->isChecked());

			std::vector<std::string> psChannelLabels;
			std::string str;

			std::istringstream iss(ui->channelLabels->toPlainText().toStdString());
			while (std::getline(iss, str, '\n'))
				psChannelLabels.push_back(str);
			//boost::algorithm::split(psChannelLabels,ui->channelLabels->toPlainText().toStdString(),boost::algorithm::is_any_of("\n"));
			ampConfiguration.m_psChannelLabels = psChannelLabels;

			std::vector<std::string> psEegChannelLabels;
			std::vector<std::string> psBipolarChannelLabels;
			std::vector<std::string> psAuxChannelLabels;
			int i=0;
			for (std::vector<std::string>::iterator it = ampConfiguration.m_psChannelLabels.begin();
				it < ampConfiguration.m_psChannelLabels.end();
				++it)
			{
				if (i < ui->eegChannelCount->value())
					psEegChannelLabels.push_back(*it);
				else if (i - ui->eegChannelCount->value() < ui->bipolarChannelCount->value())
					psBipolarChannelLabels.push_back(*it);
				else
					psAuxChannelLabels.push_back(*it);
				i++;
			}
			ampConfiguration.m_psEegChannelLabels = psEegChannelLabels;
			ampConfiguration.m_psBipolarChannelLabels = psBipolarChannelLabels;
			ampConfiguration.m_psAuxChannelLabels = psAuxChannelLabels;

			if (ampConfiguration.m_psEegChannelLabels.size() != ui->eegChannelCount->value()) {
				QMessageBox::critical(this, "Error", "The number of eeg channels labels does not match the eeg channel count device setting.", QMessageBox::Ok);
				return;
			}

			if (ampConfiguration.m_psBipolarChannelLabels.size() != ui->bipolarChannelCount->value()) {
				QMessageBox::critical(this, "Error", "The number of bipolar channels labels does not match the bipolar channel count device setting.", QMessageBox::Ok);
				return;
			}
			t_VersionNumber version;
			GetLibraryVersion(&version);
			std::cout << "Library Version " << LIBVERSIONSTREAM(version) << std::endl;

			float fSamplingRate = (float) ampConfiguration.m_dSamplingRate;
			std::string sSerialNumber = ui->deviceSerialNumber->text().toStdString();
			this->setWindowTitle(QString(std::string("Connecting to "+sSerialNumber).c_str()));
			this->setCursor(Qt::WaitCursor);
			std::string error;
			int nRet = m_LiveAmp.Setup(sSerialNumber, fSamplingRate, ampConfiguration.m_bUseSampleCounter, ampConfiguration.m_bUseSim, RM_NORMAL);
			if (nRet != 0)
			{
				QMessageBox::critical(this, tr("LiveAmp Connector"),
					tr(("Cannot find device with serial number " + sSerialNumber).c_str()),
					QMessageBox::Ok);
				this->setWindowTitle("LiveAmp Connector");
				this->setCursor(Qt::ArrowCursor);
			}
			else if(ui->auxChannelCount->value() > 0 && !m_LiveAmp.hasSTE())
			{
				nRet = QMessageBox::critical(this, tr("LiveAmp Connector"),
					tr("No STE box detected. Change number of AUX channels to 0 or connect STE and try again."),
					QMessageBox::Ok);
				this->setWindowTitle("LiveAmp Connector");
				this->setCursor(Qt::ArrowCursor);
			}
			else
			{
				if (m_LiveAmp.is64())
				{
					if (fSamplingRate == 1000.0)
						nRet = QMessageBox::warning(this, tr("LiveAmp Connector"),
							tr("A sampling rate of 1kHz is not recommended for LiveAmp64"),
							QMessageBox::Ok);
				}
				m_LiveAmp.setOutTriggerMode(m_TriggerOutputMode, 8, ui->sbSyncFreq->value(), 5);

				std::vector<int> eegIndices;
				for (int i = 0; i < ampConfiguration.m_psEegChannelLabels.size(); i++)
					eegIndices.push_back(i);

				std::vector<int> bipolarIndices;
				for (int i = 0; i < ampConfiguration.m_psBipolarChannelLabels.size(); i++)
					bipolarIndices.push_back(i + 24);

				std::vector<int> auxIndices;
				for (int i = 0; i < ui->auxChannelCount->value(); i++)
					auxIndices.push_back(i + 32);
				if (ampConfiguration.m_psEegChannelLabels.size() + ampConfiguration.m_psBipolarChannelLabels.size() > 32)
					if (!m_LiveAmp.is64())
						nRet = QMessageBox::warning(this, tr("LiveAmp Connector"),
							tr("This device is a LiveAmp32 but more than 32 EEG/BiPolar channels are requested.\n"
								"If you are trying to connect to a 64 channel device, power cycle and try again."),
							QMessageBox::Ok);
				
				;// issue warning
				m_LiveAmp.enableChannels(eegIndices, bipolarIndices, auxIndices, ampConfiguration.m_bUseACC);
				this->setCursor(Qt::ArrowCursor);
				// start reader thread
				m_bStop = false;
				m_ptReaderThread.reset(new std::thread(&MainWindow::ReadThread, this, ampConfiguration));

				this->setWindowTitle(("Streaming from LiveAmp " + sSerialNumber).c_str() );
				ui->channelLabels->setEnabled(false);
				ui->overwriteChannelLabels->setEnabled(false);
				ui->STESettings->setEnabled(false);
				ui->allDeviceSettings->setEnabled(false);
				ui->linkButton->setEnabled(true);
				ui->linkButton->setText("Unlink");
			}
		}
		catch(std::exception &e) 
		{
	
			int errorcode=0; 
			//if(m_LiveAmp.getHandle()!=NULL)m_LiveAmp.close();
			QMessageBox::critical(this,"Error",(std::string("Could not perform Link action: ")+=e.what()).c_str(),QMessageBox::Ok);
			this->setWindowTitle("LiveAmp Connector");
			ui->channelLabels->setEnabled(true);
			ui->overwriteChannelLabels->setEnabled(true);
			ui->STESettings->setEnabled(true);
			ui->allDeviceSettings->setEnabled(true);
			ui->linkButton->setEnabled(true);
			ui->linkButton->setText("Link");
			this->setCursor(Qt::ArrowCursor);
			return;
		}
	}
}

void MainWindow::ReadThread(t_AmpConfiguration ampConfiguration) 
{

	lsl::stream_outlet *pMarkerOutlet = NULL;
	lsl::stream_outlet *pMarkerOutletIn = NULL;
	lsl::stream_outlet *pMarkerOutletOut = NULL;
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	int nEnabledChannelCountGUI = ampConfiguration.m_nEEGChannelCount +
		ampConfiguration.m_nBipolarChannelCount +
		ampConfiguration.m_nAuxChannelCount +
		(ampConfiguration.m_bUseACC ? 3 : 0);

	int nTriggerIdx = nEnabledChannelCountGUI;
	int nSampleCounterIdx = nTriggerIdx + 3; // 2 (0 indexed) for the 2 triggers (i/o) and 2 for the CT_DIG channels which we ignore
	int nExtraChannels = 0;
	if (ampConfiguration.m_bSampledMarkersEEG)
	{
		nExtraChannels++;
		if (m_LiveAmp.hasSTE())
		{
			nExtraChannels++;
			if (!ampConfiguration.m_bIsSTEInDefault)
				nExtraChannels++;
		}
	}
	if (ampConfiguration.m_bUseSampleCounter)
		nExtraChannels++;

	int nTotalOutputChannelCount = nEnabledChannelCountGUI + nExtraChannels;
	bool bUseSampleCounter = ampConfiguration.m_bUseSampleCounter;
	int nExtraEEGMarkerChannelCnt = 0;
	if (ampConfiguration.m_bSampledMarkersEEG)
	{
		nExtraEEGMarkerChannelCnt = 1;
		if (m_LiveAmp.hasSTE())
		{
			nExtraEEGMarkerChannelCnt = 2;
			if (!ui->rbDefault->isChecked())
				nExtraEEGMarkerChannelCnt = 3;
		}
	}

	std::vector<float> pfSampleBuffer(nTotalOutputChannelCount);
	std::vector<std::vector<float>> ppfLiveAmpBuffer(ampConfiguration.m_nChunkSize,std::vector<float>(m_LiveAmp.getEnabledChannelCnt()));
	std::vector<std::vector<float>> ppfChunkBuffer(ampConfiguration.m_nChunkSize,std::vector<float>(nTotalOutputChannelCount));
	std::vector<int16_t> pnUnsampledMarkerBuffer(ampConfiguration.m_nChunkSize);
	std::vector<int16_t> pnUnsampledMarkerBufferIn(ampConfiguration.m_nChunkSize);
	std::vector<int16_t> pnUnsampledMarkerBufferOut(ampConfiguration.m_nChunkSize);

	float fMrkr;
	float fPrevMarker=0.0;
	float fMrkrIn;
	float fPrevMarkerIn = 0.0;
	float fMrkrOut;
	float fPrevMarkerOut = 0.0;
	float fUMrkr;
	float fPrevUMarker = 0.0;
	float fUMrkrIn;
	float fPrevUMarkerIn = 0.0;
	float fUMrkrOut;
	float fPrevUMarkerOut = 0.0;

	BYTE* pBuffer = NULL;

	try 
	{
		m_LiveAmp.startAcquisition();
		std::vector<int>pnTriggerIndeces = m_LiveAmp.getTrigIndices();
		lsl::stream_info dataInfo("LiveAmpSN-" + m_LiveAmp.getSerialNumber(),"EEG", nTotalOutputChannelCount, ampConfiguration.m_dSamplingRate, lsl::cf_float32,"LiveAmpSN-" + m_LiveAmp.getSerialNumber());
		lsl::xml_element channels = dataInfo.desc().append_child("channels");

		for (std::size_t k = 0; k < ampConfiguration.m_psChannelLabels.size(); k++) 
		{
			if (k < ampConfiguration.m_nEEGChannelCount)

			{
				channels.append_child("channel")
					.append_child_value("label", ampConfiguration.m_psChannelLabels[k].c_str())
					.append_child_value("type", "EEG")
					.append_child_value("unit", "microvolts");
			}
			else if (k < ampConfiguration.m_nEEGChannelCount + ampConfiguration.m_nBipolarChannelCount)
			{
				channels.append_child("channel")
					.append_child_value("label", ampConfiguration.m_psChannelLabels[k].c_str())
					.append_child_value("type", "bipolar")
					.append_child_value("unit", "microvolts");
			}
			else if (k < ampConfiguration.m_nEEGChannelCount + ampConfiguration.m_nBipolarChannelCount + ampConfiguration.m_nAuxChannelCount)
			{

				channels.append_child("channel")
					.append_child_value("label", ampConfiguration.m_psChannelLabels[k].c_str())
					.append_child_value("type", "AUX")
					.append_child_value("unit", "microvolts");
			}
			else 
			{
				channels.append_child("channel")
					.append_child_value("label", ampConfiguration.m_psChannelLabels[k].c_str())
					.append_child_value("type", "ACC")
					.append_child_value("unit", "milliGs");
			}
		}
		
		if (bUseSampleCounter)
		{
			channels.append_child("channel")
				.append_child_value("label", "SampleCounter")
				.append_child_value("type", "SampleCounter")
				.append_child_value("unit", "");
		}

		if (ampConfiguration.m_bSampledMarkersEEG)
		{
			channels.append_child("channel")
				.append_child_value("label", "DeviceTrigger")
				.append_child_value("type", "DeviceTrigger")
				.append_child_value("unit", "");
		}

		// only create this channel if the STE is connected
		if (ampConfiguration.m_bSampledMarkersEEG && m_LiveAmp.hasSTE()) {
			channels.append_child("channel")
				.append_child_value("label", "STETriggerIn")
				.append_child_value("type", "STETriggerIn")
				.append_child_value("unit", "integer");
		}


		if (ampConfiguration.m_bSampledMarkersEEG && m_LiveAmp.hasSTE() && !ampConfiguration.m_bIsSTEInDefault)
		{
			channels.append_child("channel")
				.append_child_value("label", "STETriggerOut")
				.append_child_value("type", "STETriggerOut")
				.append_child_value("unit", "integer");
		}

		dataInfo.desc().append_child("acquisition")
			.append_child_value("manufacturer","Brain Products");

		t_VersionNumber libVersion;
		GetLibraryVersion(&libVersion);
		int32_t lslProtocolVersion = lsl::protocol_version();
		int32_t lslLibVersion = lsl::library_version();
		std::stringstream ssLib;
		ssLib << LIBVERSIONSTREAM(libVersion);
		std::stringstream ssProt;
		ssProt << LSLVERSIONSTREAM(lslProtocolVersion);
		std::stringstream ssLSL;
		ssLSL << LSLVERSIONSTREAM(lslLibVersion);
		std::stringstream ssApp;
		ssApp << APPVERSIONSTREAM(m_AppVersion);
		// make a data outlet
		lsl::stream_outlet dataOutlet(dataInfo);

		dataInfo.desc().append_child("versions")
			.append_child_value("Amplifier_LIB", ssLib.str())
			.append_child_value("lsl_protocol", ssProt.str())
			.append_child_value("liblsl", ssLSL.str())
			.append_child_value("App", ssApp.str() + "_beta");

		// create marker streaminfo and outlet
		if(ampConfiguration.m_bUnsampledMarkers) {
			lsl::stream_info markerInfo("LiveAmpSN-" + m_LiveAmp.getSerialNumber() + "-DeviceTrigger","Markers", 1, 0, lsl::cf_string,"LiveAmpSN-" + m_LiveAmp.getSerialNumber() + "_DeviceTrigger");
			pMarkerOutlet = new lsl::stream_outlet(markerInfo);

			if (m_LiveAmp.hasSTE())
			{
				lsl::stream_info markerInfoIn("LiveAmpSN-" + m_LiveAmp.getSerialNumber() + "-STETriggerIn", "Markers", 1, 0, lsl::cf_string, "LiveAmpSN-" + m_LiveAmp.getSerialNumber() + "_STETriggerIn");
				pMarkerOutletIn = new lsl::stream_outlet(markerInfoIn);
				if (!ampConfiguration.m_bIsSTEInDefault)
				{
					lsl::stream_info markerInfoOut("LiveAmpSN-" + m_LiveAmp.getSerialNumber() + "-STETriggerOut", "Markers", 1, 0, lsl::cf_string, "LiveAmpSN-" + m_LiveAmp.getSerialNumber() + "_STETriggerOut");
					pMarkerOutletOut = new lsl::stream_outlet(markerInfoOut);
				}
			}
		}	

		int nLastMrk = 0;
		int32_t nBufferSize = (ampConfiguration.m_nChunkSize) * m_LiveAmp.getSampleSize();
		pBuffer =  new BYTE[nBufferSize];
		ppfChunkBuffer.clear();
		int64_t nSamplesRead;
		int nSampleCount;

		while (!m_bStop) {
			nSamplesRead = m_LiveAmp.pullAmpData(pBuffer, nBufferSize);
			if (nSamplesRead <= 0){
				// CPU saver, this is ok even at higher sampling rates
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			m_LiveAmp.pushAmpData(pBuffer, nBufferSize, nSamplesRead, ppfLiveAmpBuffer);
			double dNow = lsl::local_clock();
			nSampleCount = (int)ppfLiveAmpBuffer.size();
			
			if(nSampleCount >= ampConfiguration.m_nChunkSize){
			
				int k;
				int i;

				for (i=0;i<nSampleCount;i++) {
					pfSampleBuffer.clear();

					for (k=0; k<nEnabledChannelCountGUI; k++)	
						pfSampleBuffer.push_back(ppfLiveAmpBuffer[i][k]); 
					if (ampConfiguration.m_bUseSampleCounter)
						pfSampleBuffer.push_back(ppfLiveAmpBuffer[i][nSampleCounterIdx]);
					// if the trigger is a new value, record it, else it is 0.0
					// totalChannelCount is always equivalent to the last channel in the liveamp_buffer
					// which corresponds to the output trigger, the one before it is the input trigger
					float fMrkrTmp = (float)(1-((int)ppfLiveAmpBuffer[i][nTriggerIdx] % 2)); // only 1 bit
					fMrkr = (fMrkrTmp == fPrevMarker ? -1.0 : (float)((int)ppfLiveAmpBuffer[i][nTriggerIdx] % 2));
					fPrevMarker = fMrkrTmp;
					if(ampConfiguration.m_bSampledMarkersEEG)
						pfSampleBuffer.push_back(fMrkr);

					if (m_LiveAmp.hasSTE() && ampConfiguration.m_bSampledMarkersEEG)
					{
						float fMrkrTmpIn = (float)(((int)ppfLiveAmpBuffer[i][nTriggerIdx] >> 8));
						fMrkrIn = (fMrkrTmpIn == fPrevMarkerIn ? -1.0 : fMrkrTmpIn);
						fPrevMarkerIn = fMrkrTmpIn;

						if (ampConfiguration.m_bSampledMarkersEEG)
							pfSampleBuffer.push_back(fMrkrIn);

						//if (!ui->rbDefault->isChecked())
						if (!ampConfiguration.m_bIsSTEInDefault)
						{
							float fMrkrTmpOut = (float)(((int)ppfLiveAmpBuffer[i][nTriggerIdx + 1] >> 8));
							fMrkrOut = (fMrkrTmpOut == fPrevMarkerOut ? -1.0 : fMrkrTmpOut);
							fPrevMarkerOut = fMrkrTmpOut;

							if (ampConfiguration.m_bSampledMarkersEEG)
								pfSampleBuffer.push_back(fMrkrOut);
						}
					}
					ppfChunkBuffer.push_back(pfSampleBuffer);
				}
				dataOutlet.push_chunk(ppfChunkBuffer, dNow);
				ppfChunkBuffer.clear();

				if(ampConfiguration.m_bUnsampledMarkers) 
				{
					for(int s=0;s<nSampleCount;s++)
					{
						// shift left to 0 out the top 8 bits, then shift right to return and keep the lower 8
						// subtract from 1 because the bit order goes from right to left
						fUMrkr = (float)(1 - (int)ppfLiveAmpBuffer[s][nTriggerIdx] % 2);
						if (fUMrkr != fPrevUMarker)
						{
							std::string sMrkr = std::to_string((int)fUMrkr);
							pMarkerOutlet->push_sample(&sMrkr, dNow + (double)(s + 1 - nSampleCount) / ampConfiguration.m_dSamplingRate);
						}
						fPrevUMarker = fUMrkr;
						if (m_LiveAmp.hasSTE())
						{
							fUMrkrIn = (float)(((int)ppfLiveAmpBuffer[s][nTriggerIdx] >> 8));
							if (fUMrkrIn != fPrevUMarkerIn) 
							{
								std::string sMrkrIn = std::to_string((int)fUMrkrIn);
								pMarkerOutletIn->push_sample(&sMrkrIn, dNow + (double)(s + 1 - nSampleCount) / ampConfiguration.m_dSamplingRate);
							}
							fPrevUMarkerIn = fUMrkrIn;
							if (ampConfiguration.m_bIsSTEInDefault)
							{
								fUMrkrOut = (float)(((int)ppfLiveAmpBuffer[s][nTriggerIdx + 1] >> 8));
								if (fUMrkrOut != fPrevUMarkerOut) 
								{
									std::string sMrkrOut = std::to_string((int)fUMrkrOut);
									pMarkerOutletOut->push_sample(&sMrkrOut, dNow + (double)(s + 1 - nSampleCount) / ampConfiguration.m_dSamplingRate);
								}
								fPrevUMarkerOut = fUMrkrOut;
							}
						}
					}
				}	
				ppfLiveAmpBuffer.clear();
			}
		}
	}

	catch (std::exception & e)
	{
		throw std::runtime_error((std::string("Acquisition loop failure: ") += e.what()).c_str());
	}

	if(pBuffer != NULL)
		delete[] pBuffer;
	pBuffer = NULL;
	// cleanup (if necessary)
	if (ampConfiguration.m_bUnsampledMarkers)
	{
		delete(pMarkerOutlet);
		if (m_LiveAmp.hasSTE())
		{
			delete(pMarkerOutletIn);
			if (!ui->rbDefault->isChecked())
				delete(pMarkerOutletOut);
		}

	}
	try
	{
		m_LiveAmp.stopAcquisition(); 
		m_LiveAmp.close();
	}
	catch(std::exception &e) 
	{
		throw std::runtime_error((std::string("Error disconnecting from LiveAmp: ") += e.what()).c_str());
	}
}



MainWindow::~MainWindow() {

	delete ui;
}

