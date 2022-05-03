
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCore/QtCore>
#include <iostream>
#include <sstream>
#include "LiveAmp.h"

#if _WIN64
#define BITDEPTH "64"
#else
#define BITDEPTH "32"
#endif

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
	m_AppVersion.Minor = 20;
	m_AppVersion.Bugfix = 4;

	m_bOverrideAutoUpdate = false;
	ui->setupUi(this);

	QObject::connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
	QObject::connect(ui->linkButton, SIGNAL(clicked()), this, SLOT(Link()));
	QObject::connect(ui->actionLoad_Configuration, SIGNAL(triggered()), this, SLOT(LoadConfigDialog()));
	QObject::connect(ui->actionSave_Configuration, SIGNAL(triggered()), this, SLOT(SaveConfigDialog()));
	QObject::connect(ui->actionVersions, SIGNAL(triggered()), this, SLOT(VersionsDialog()));
	QObject::connect(ui->refreshDevices,SIGNAL(clicked()),this,SLOT(RefreshDevices()));
	QObject::connect(ui->eegChannelCount, SIGNAL(valueChanged(int)),this, SLOT(UpdateChannelLabelsWithEeg(int)));
	QObject::connect(ui->deviceCb,SIGNAL(currentIndexChanged(int)),this,SLOT(ChooseDevice(int)));
	QObject::connect(ui->auxChannelCount, SIGNAL(valueChanged(int)), this, SLOT(UpdateChannelLabelsAux(int)));
	QObject::connect(ui->rbSync, SIGNAL(clicked(bool)), this, SLOT(RadioButtonBehavior(bool)));
	QObject::connect(ui->rbMirror, SIGNAL(clicked(bool)), this, SLOT(RadioButtonBehavior(bool)));
	QString cfgfilepath = FindConfigFile(config_file);
	LoadConfig(cfgfilepath);
}

void MainWindow::UpdateChannelLabels(void) 
{
	if (!ui->overwriteChannelLabels->isChecked())return;
	int nEeg = ui->eegChannelCount->value();
	int nAux = ui->auxChannelCount->value();
	int nAcc = (ui->useACC->isChecked()) ? 3 : 0;
	bool bUseSampleCounter = ui->sampleCounter->isChecked();
	int nTotalElectrodes = nEeg;
	std::string str;
	std::vector<std::string> psEEGChannelLabels;
	std::istringstream iss(ui->channelLabels->toPlainText().toStdString()); 
	while (std::getline(iss, str, '\n'))
		psEEGChannelLabels.push_back(str);
	int nVal = ui->eegChannelCount->value();
	for (auto it = psEEGChannelLabels.begin(); it != psEEGChannelLabels.end();)
	{
		if ((*(it)).find("AUX") != std::string::npos)
			it = psEEGChannelLabels.erase(it);
		else
			++it;
	}
	while (int i = psEEGChannelLabels.size() > nVal)
		psEEGChannelLabels.pop_back();
	if(psEEGChannelLabels.size()>0)
		while (psEEGChannelLabels[psEEGChannelLabels.size() - 1].find("*") != std::string::npos)
			psEEGChannelLabels.pop_back();

	ui->channelLabels->clear();
	for (int i = 0; i < nTotalElectrodes; i++) {
		if (i < psEEGChannelLabels.size())
			str = psEEGChannelLabels[i];
		else
			str = std::to_string(i);
		ui->channelLabels->appendPlainText(str.c_str());
	}

	for (int i = 1; i <= nAux; i++) 
	{
		str = "AUX_" + std::to_string(i);
		ui->channelLabels->appendPlainText(str.c_str());
	}
}

void MainWindow::UpdateChannelLabelsWithEeg(int n)
{
	
	if (m_bOverrideAutoUpdate)return;
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
		  "App: " << APPVERSIONSTREAM(m_AppVersion) << ", " << BITDEPTH << "-bit";
	QMessageBox::information(this, "Versions", ss.str().c_str(), QMessageBox::Ok);
}

void MainWindow::UpdateChannelLabelsAux(int) 
{
	//if (!ui->overwriteChannelLabels->isChecked())return;
	UpdateChannelLabels();
}

void MainWindow::closeEvent(QCloseEvent *ev) 
{
	if (m_ptReaderThread)
		ev->ignore();
}

void MainWindow::LoadConfig(const QString& filename) 
{
	QSettings pt(filename, QSettings::IniFormat);

	ui->deviceSerialNumber->setText(pt.value("settings/serialnumber", "x-0077").toString());
	int nChannelCount = pt.value("settings/channelcount", 32).toInt();
	ui->eegChannelCount->setMaximum((nChannelCount>32)?64:32);
	ui->eegChannelCount->setValue(pt.value("settings/channelcount", 32).toInt());
	m_nEegChannelCount = ui->eegChannelCount->value();
	ui->chunkSize->setValue(pt.value("settings/chunksize", 10).toInt());
	int idx = getSamplingRateIndex(pt.value("settings/samplingrate", 250).toInt());
	ui->samplingRate->setCurrentIndex(idx);
	ui->auxChannelCount->setValue(pt.value("settings/auxChannelCount", 0).toInt());
	int nAuxCount = ui->auxChannelCount->value();
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
	pt.setValue("serialnumber", ui->deviceSerialNumber->text());
	pt.setValue("channelcount", ui->eegChannelCount->value());
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

void MainWindow::HandleListenerException(std::exception e)
{
	QMessageBox::critical(this, "Error", (std::string("Error in acquisition loop: ") += e.what()).c_str(), QMessageBox::Ok);
	ResetGuiEnabling(true);
}

void MainWindow::ResetGuiEnabling(bool b)
{
	if (b == false)
	{
		ui->channelLabels->setEnabled(false);
		ui->overwriteChannelLabels->setEnabled(false);
		ui->STESettings->setEnabled(false);
		ui->allDeviceSettings->setEnabled(false);
		ui->scanGroup->setEnabled(false);
		ui->triggerStyleGroup->setEnabled(false);
		ui->linkButton->setEnabled(true);
		ui->linkButton->setText("Unlink");
	}
	else
	{
		ui->channelLabels->setEnabled(true);
		ui->overwriteChannelLabels->setEnabled(true);
		ui->STESettings->setEnabled(true);
		ui->allDeviceSettings->setEnabled(true);
		ui->scanGroup->setEnabled(true);
		ui->triggerStyleGroup->setEnabled(true);
		ui->linkButton->setEnabled(true);
		ui->linkButton->setText("Link");
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
	try
	{
		LiveAmp::enumerate(ampData, ui->useSim->checkState());
	}
	catch(std::exception &e) 
	{
		QMessageBox::critical(this,"Error",(std::string("Could not locate LiveAmp device(s): ")+=e.what()).c_str(),QMessageBox::Ok);
	}
	this->setCursor(Qt::ArrowCursor);
	if(!m_psLiveAmpSns.empty())m_psLiveAmpSns.clear();
	if(!m_pnUsableChannelsByDevice.empty()) m_pnUsableChannelsByDevice.clear();
	if(!ampData.empty()) {
		ui->deviceCb->clear();
		std::stringstream ss;
		int i=0;
		
		for(std::vector<std::pair<std::string, int>>::iterator it=ampData.begin(); it!=ampData.end();++it){
			ss.clear();
			ss << it->first << " (" << it->second << ")";
			std::cout<<it->first<<std::endl;
			auto x = ss.str();
			qsl << QString(ss.str().c_str());
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
	ui->eegChannelCount->setMaximum(m_nEegChannelCount);
	if (!ui->overwriteChannelLabels->isChecked())return;
	ui->eegChannelCount->setValue(m_nEegChannelCount);
	UpdateChannelLabels();
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

		} 
		catch(std::exception &e) 
		{
			QMessageBox::critical(this,"Error",(std::string("Could not stop the background processing: ")+=e.what()).c_str(),QMessageBox::Ok);
			return;
		}
		this->setWindowTitle("LiveAmp Connector");
		ResetGuiEnabling(true);
	}
	else 
	{
		try 
		{
			t_AmpConfiguration ampConfiguration;
			ampConfiguration.m_sSerialNumber = ui->deviceSerialNumber->text().toStdString();
			ampConfiguration.m_nEEGChannelCount = ui->eegChannelCount->value();
			ampConfiguration.m_nAuxChannelCount = ui->auxChannelCount->value();
			ampConfiguration.m_nChunkSize = ui->chunkSize->value();
			ampConfiguration.m_dSamplingRate = (double)pnSamplingRates[ui->samplingRate->currentIndex()];
			ampConfiguration.m_bUseACC = ui->useACC->checkState() == Qt::Checked;
			ampConfiguration.m_bUseSampleCounter = ui->sampleCounter->checkState() == Qt::Checked;
			ampConfiguration.m_bUseSim = ui->useSim->checkState() == Qt::Checked;
			ampConfiguration.m_bUnsampledMarkers = ui->unsampledMarkers->checkState() == Qt::Checked;
			ampConfiguration.m_bSampledMarkersEEG = ui->sampledMarkersEEG->checkState()==Qt::Checked;
			ampConfiguration.m_bIsSTEInDefault = (ui->rbDefault->isChecked());
			ampConfiguration.m_bIsSTEInMirror = (ui->rbMirror->isChecked());
			ampConfiguration.m_bIsSTEInSync = (ui->rbSync->isChecked());

			std::vector<std::string> psChannelLabels;
			std::string str;

			std::istringstream iss(ui->channelLabels->toPlainText().toStdString());
			while (std::getline(iss, str, '\n'))
				psChannelLabels.push_back(str);
			ampConfiguration.m_psChannelLabels = psChannelLabels;

			std::vector<std::string> psEegChannelLabels;
			std::vector<std::string> psAuxChannelLabels;
			int i=0;
			for (std::vector<std::string>::iterator it = ampConfiguration.m_psChannelLabels.begin();
				it < ampConfiguration.m_psChannelLabels.end();
				++it)
			{
				if (i < ui->eegChannelCount->value())
					psEegChannelLabels.push_back(*it);
				else
					psAuxChannelLabels.push_back(*it);
				i++;
			}
			ampConfiguration.m_psEegChannelLabels = psEegChannelLabels;
			ampConfiguration.m_psAuxChannelLabels = psAuxChannelLabels;

			t_VersionNumber version;
			GetLibraryVersion(&version);
			std::cout << "Library Version " << LIBVERSIONSTREAM(version) << std::endl;

			float fSamplingRate = (float) ampConfiguration.m_dSamplingRate;
			std::string sSerialNumber = ui->deviceSerialNumber->text().toStdString();
			this->setWindowTitle(QString(std::string("Connecting to "+sSerialNumber).c_str()));
			this->setCursor(Qt::WaitCursor);
			std::string error;
			m_pLiveAmp.reset(new LiveAmp(sSerialNumber, fSamplingRate, ampConfiguration.m_bUseSampleCounter, ampConfiguration.m_bUseSim, RM_NORMAL));

			if(ui->auxChannelCount->value() > 0 && !m_pLiveAmp->hasSTE())
			{
				QMessageBox::critical(this, tr("LiveAmp Connector"),
					tr("No STE box detected. Change number of AUX channels to 0 or connect STE and try again."),
					QMessageBox::Ok);
				this->setWindowTitle("LiveAmp Connector");
				this->setCursor(Qt::ArrowCursor);
			}
			else
			{
				if (m_pLiveAmp->is64())
				{
					if (fSamplingRate == 1000.0)
						QMessageBox::warning(this, tr("LiveAmp Connector"),
							tr("A sampling rate of 1kHz is not recommended for LiveAmp64"),
							QMessageBox::Ok);
				}
				int nPulseWidth = (int)(.01 * fSamplingRate);
				m_pLiveAmp->setOutTriggerMode(m_TriggerOutputMode, 0, ui->sbSyncFreq->value(), nPulseWidth);

				std::vector<int> eegIndices;
				for (int i = 0; i < ampConfiguration.m_psEegChannelLabels.size(); i++)
					eegIndices.push_back(i);

				std::vector<int> auxIndices;
				for (int i = 0; i < ui->auxChannelCount->value(); i++)
					auxIndices.push_back(i);
				if (ampConfiguration.m_psEegChannelLabels.size() > 32)
					if (!m_pLiveAmp->is64())
						QMessageBox::warning(this, tr("LiveAmp Connector"),
							tr("The current device being linked is not a LiveAmp64, but more than 32 EEG/BiPolar channels are requested.\n"
								"If you are trying to connect to a 64 channel device, power cycle and try again."),
							QMessageBox::Ok);
				m_pLiveAmp->enableChannels(eegIndices, auxIndices, ampConfiguration.m_bUseACC);
				this->setCursor(Qt::ArrowCursor);
				// start reader thread
				m_bStop = false;
				m_ptReaderThread.reset(new std::thread(&MainWindow::ReadThread, this, ampConfiguration));
				this->setWindowTitle(("Streaming from LiveAmp " + sSerialNumber).c_str() );
				ResetGuiEnabling(false);
			}
		}
		catch(std::exception &e) 
		{
			int errorcode=0; 
			QMessageBox::critical(this,"Error",(std::string("Could not perform Link action: ")+=e.what()).c_str(),QMessageBox::Ok);
			this->setWindowTitle("LiveAmp Connector");
			ResetGuiEnabling(true);
			this->setCursor(Qt::ArrowCursor);
			m_pLiveAmp->close();
			return;
		}
	}
}

void MainWindow::ReadThread(t_AmpConfiguration ampConfiguration) 
{

	std::unique_ptr<lsl::stream_outlet> pMarkerOutlet;
	std::unique_ptr<lsl::stream_outlet> pMarkerOutletSTE;
	std::unique_ptr<lsl::stream_outlet> pMarkerOutletSync;
	SetPriorityClass(GetCurrentThread(), HIGH_PRIORITY_CLASS);

	int nChannelLabelCount = ampConfiguration.m_nEEGChannelCount +
		ampConfiguration.m_nAuxChannelCount;
		
	int nTriggerIdx = nChannelLabelCount + (ampConfiguration.m_bUseACC ? 3 : 0);
	int nSampleCounterIdx = nTriggerIdx + 3; // 2 (0 indexed) for the 2 triggers (i/o) and 2 for the CT_DIG channels which we ignore
	int nExtraChannels = (ampConfiguration.m_bUseACC ? 3 : 0);
	if (ampConfiguration.m_bSampledMarkersEEG)
	{
		nExtraChannels++;
		if (m_pLiveAmp->hasSTE())
		{
			nExtraChannels++;
			if (ampConfiguration.m_bIsSTEInSync)
				nExtraChannels++;
		}
	}
	if (ampConfiguration.m_bUseSampleCounter)
		nExtraChannels++;

	int nTotalOutputChannelCount = nChannelLabelCount + nExtraChannels;
	int nEEGAuxAndAccChannelCount = nChannelLabelCount + (ampConfiguration.m_bUseACC ? 3 : 0);
	std::vector<float> pfSampleBuffer(nTotalOutputChannelCount);
	std::vector<std::vector<float>> ppfLiveAmpBuffer(ampConfiguration.m_nChunkSize,std::vector<float>(m_pLiveAmp->getEnabledChannelCnt()));
	std::vector<std::vector<float>> ppfChunkBuffer(ampConfiguration.m_nChunkSize,std::vector<float>(nTotalOutputChannelCount));

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
		m_pLiveAmp->startAcquisition();
		std::vector<int>pnTriggerIndeces = m_pLiveAmp->getTrigIndices();
		lsl::stream_info dataInfo("LiveAmpSN-" + m_pLiveAmp->getSerialNumber(),"EEG", nTotalOutputChannelCount, ampConfiguration.m_dSamplingRate, lsl::cf_float32,"LiveAmpSN-" + m_pLiveAmp->getSerialNumber());
		lsl::xml_element channels = dataInfo.desc().append_child("channels");

		for (int k = 0; k < nChannelLabelCount; k++)
		{
			if (k < ampConfiguration.m_nEEGChannelCount)

			{
				channels.append_child("channel")
					.append_child_value("label", ampConfiguration.m_psChannelLabels[k].c_str())
					.append_child_value("type", "EEG")
					.append_child_value("unit", "microvolts");
			}
			else
			{
				channels.append_child("channel")
					.append_child_value("label", ampConfiguration.m_psChannelLabels[k].c_str())
					.append_child_value("type", "AUX")
					.append_child_value("unit", "microvolts");
			}
		}

		if(ampConfiguration.m_bUseACC)
		{
			channels.append_child("channel")
				.append_child_value("label", "ACC_X")
				.append_child_value("type", "ACC")
				.append_child_value("unit", "milliGs");
			channels.append_child("channel")
				.append_child_value("label", "ACC_Y")
				.append_child_value("type", "ACC")
				.append_child_value("unit", "milliGs");
			channels.append_child("channel")
				.append_child_value("label", "ACC_Z")
				.append_child_value("type", "ACC")
				.append_child_value("unit", "milliGs");
		}
	
		if (ampConfiguration.m_bUseSampleCounter)
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
				.append_child_value("type", "Trigger")
				.append_child_value("unit", "integer");
		}

		// only create this channel if the STE is connected
		if (ampConfiguration.m_bSampledMarkersEEG && m_pLiveAmp->hasSTE()) {
			channels.append_child("channel")
				.append_child_value("label", "STETriggerIn")
				.append_child_value("type", "Trigger")
				.append_child_value("unit", "integer");
		}


		if (ampConfiguration.m_bSampledMarkersEEG && m_pLiveAmp->hasSTE() && ampConfiguration.m_bIsSTEInSync)
		{
			channels.append_child("channel")
				.append_child_value("label", "STESync")
				.append_child_value("type", "SyncPulse")
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
			.append_child_value("App", ssApp.str());

		// create marker streaminfo and outlet
		if(ampConfiguration.m_bUnsampledMarkers) {
			lsl::stream_info markerInfo("LiveAmpSN-" + m_pLiveAmp->getSerialNumber() + "-DeviceTrigger","Markers", 1, 0, lsl::cf_string,"LiveAmpSN-" + m_pLiveAmp->getSerialNumber() + "_DeviceTrigger");
			pMarkerOutlet.reset(new lsl::stream_outlet(markerInfo));

			if (m_pLiveAmp->hasSTE())
			{
				lsl::stream_info markerInfoSTE("LiveAmpSN-" + m_pLiveAmp->getSerialNumber() + "-STETriggerIn", "Markers", 1, 0, lsl::cf_string, "LiveAmpSN-" + m_pLiveAmp->getSerialNumber() + "_STETriggerIn");
				pMarkerOutletSTE.reset(new lsl::stream_outlet(markerInfoSTE));
				if (ampConfiguration.m_bIsSTEInSync)
				{
					lsl::stream_info markerInfoSync("LiveAmpSN-" + m_pLiveAmp->getSerialNumber() + "-STESync", "Markers", 1, 0, lsl::cf_string, "LiveAmpSN-" + m_pLiveAmp->getSerialNumber() + "_STESync");
					pMarkerOutletSync.reset(new lsl::stream_outlet(markerInfoSync));
				}
			}
		}	

		int nLastMrk = 0;
		int32_t nBufferSize = (ampConfiguration.m_nChunkSize) * m_pLiveAmp->getSampleSize();
		pBuffer =  new BYTE[nBufferSize];
		ppfChunkBuffer.clear();
		int64_t nSamplesRead;
		int nSampleCount;

		while (!m_bStop) {
			nSamplesRead = m_pLiveAmp->pullAmpData(pBuffer, nBufferSize);
			if (nSamplesRead <= 0){
				// CPU saver, this is ok even at higher sampling rates
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			m_pLiveAmp->pushAmpData(pBuffer, nBufferSize, nSamplesRead, ppfLiveAmpBuffer);
			double dNow = lsl::local_clock();
			nSampleCount = (int)ppfLiveAmpBuffer.size();
			
			if(nSampleCount >= ampConfiguration.m_nChunkSize){
			
				int k;
				int i;

				for (i=0;i<nSampleCount;i++) {
					pfSampleBuffer.clear();

					for (k=0; k<nEEGAuxAndAccChannelCount; k++)	
						pfSampleBuffer.push_back(ppfLiveAmpBuffer[i][k]); 
					if (ampConfiguration.m_bUseSampleCounter)
						pfSampleBuffer.push_back(ppfLiveAmpBuffer[i][nSampleCounterIdx]);
					// if the trigger is a new value, record it, else it is 0.0
					// totalChannelCount is always equivalent to the last channel in the liveamp_buffer
					// which corresponds to the output trigger, the one before it is the input trigger
					float fMrkrTmp = (float)(1-((int)ppfLiveAmpBuffer[i][nTriggerIdx] % 2)); // only 1 bit
					fMrkr = (fMrkrTmp == fPrevMarker ? -1.0 : (float)((int)1-(int)ppfLiveAmpBuffer[i][nTriggerIdx] % 2));
					fPrevMarker = fMrkrTmp;
					if(ampConfiguration.m_bSampledMarkersEEG)
						pfSampleBuffer.push_back(fMrkr);

					if (m_pLiveAmp->hasSTE() && ampConfiguration.m_bSampledMarkersEEG)
					{
						float fMrkrTmpIn = (float)(((int)ppfLiveAmpBuffer[i][nTriggerIdx] >> 8));
						fMrkrIn = (fMrkrTmpIn == fPrevMarkerIn ? -1.0 : fMrkrTmpIn);
						fPrevMarkerIn = fMrkrTmpIn;

						if (ampConfiguration.m_bSampledMarkersEEG)
							pfSampleBuffer.push_back(fMrkrIn);

						if (ampConfiguration.m_bIsSTEInSync)
						{
							float fMrkrTmpOut = (float)(((int)ppfLiveAmpBuffer[i][nTriggerIdx + (int)1] >> 8));
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
						fUMrkr = (float)((int)ppfLiveAmpBuffer[s][nTriggerIdx] % 2);
						if (fUMrkr != fPrevUMarker)
						{
							std::string sMrkr = std::to_string((int)fUMrkr);
							pMarkerOutlet->push_sample(&sMrkr, dNow + (double)(s + (int)1 - nSampleCount) / ampConfiguration.m_dSamplingRate);
						}
						fPrevUMarker = fUMrkr;
						if (m_pLiveAmp->hasSTE())
						{
							fUMrkrIn = (float)(((int)ppfLiveAmpBuffer[s][nTriggerIdx] >> 8));
							if (fUMrkrIn != fPrevUMarkerIn) 
							{
								std::string sMrkrIn = std::to_string((int)fUMrkrIn);
								pMarkerOutletSTE->push_sample(&sMrkrIn, dNow + (double)(s + (int)1 - nSampleCount) / ampConfiguration.m_dSamplingRate);
							}
							fPrevUMarkerIn = fUMrkrIn;
							if (ampConfiguration.m_bIsSTEInSync)
							{
								fUMrkrOut = (float)(((int)ppfLiveAmpBuffer[s][nTriggerIdx + (int)1] >> 8));
								if (fUMrkrOut != fPrevUMarkerOut) 
								{
									std::string sMrkrOut = std::to_string((int)fUMrkrOut);
									pMarkerOutletSync->push_sample(&sMrkrOut, dNow + (double)(s + (int)1 - nSampleCount) / ampConfiguration.m_dSamplingRate);
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
	
	try
	{
		m_pLiveAmp->stopAcquisition(); 
		m_pLiveAmp->close();
	}
	catch(std::exception &e) 
	{
		throw std::runtime_error((std::string("Error disconnecting from LiveAmp: ") += e.what()).c_str());
	}
}

QString MainWindow::FindConfigFile(const char* filename) {
	if (filename) {
		QString qfilename(filename);
		if (!QFileInfo::exists(qfilename))
			QMessageBox(QMessageBox::Warning, "Config file not found",
				QStringLiteral("The file '%1' doesn't exist").arg(qfilename), QMessageBox::Ok,
				this);
		else
			return qfilename;
	}
	QFileInfo exeInfo(QCoreApplication::applicationFilePath());
	QString defaultCfgFilename(exeInfo.completeBaseName() + ".cfg");
	QStringList cfgpaths;
	cfgpaths << QDir::currentPath()
		<< QStandardPaths::standardLocations(QStandardPaths::ConfigLocation) << exeInfo.path();
	for (auto path : cfgpaths) {
		QString cfgfilepath = path + QDir::separator() + defaultCfgFilename;
		if (QFileInfo::exists(cfgfilepath)) return cfgfilepath;
	}
	QMessageBox(QMessageBox::Warning, "No config file not found",
		QStringLiteral("No default config file could be found"), QMessageBox::Ok, this);
	return "";
}

MainWindow::~MainWindow() 
{
	delete ui;
}


