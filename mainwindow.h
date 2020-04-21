#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QThread>
#include <thread>
#include <string>
#include <vector>

// LSL API
#define LSL_DEBUG_BINDINGS
#include <lsl_cpp.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinIoCtl.h>

#include "LiveAmp.h"

namespace Ui {
class MainWindow;
}

class WaitThread : public QThread
{
	Q_OBJECT

protected:
	void run();
};



class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent, const char* config_file);
    ~MainWindow();
    

private slots:

    void LoadConfigDialog();
    void SaveConfigDialog();
	void VersionsDialog();
	void RefreshDevices();
	void Link();
    void CloseEvent(QCloseEvent *ev);
	void UpdateChannelLabelsWithEeg(int);
	void UpdateChannelLabelsWithBipolar(int);
	void UpdateChannelLabelsAux(int);
	void UpdateChannelLabelsAcc(bool);
	void UpdateChannelLabelsSampleCounter(bool);
	void ChooseDevice(int which);
	void RadioButtonBehavior(bool b);

private:

	struct t_AppVersion
	{
		int32_t Major;
		int32_t Minor;
		int32_t Bugfix;
	};
	t_AppVersion m_AppVersion;

	struct t_AmpConfiguration
	{
		std::string m_sSerialNumber;
		int m_nEEGChannelCount;
		int m_nBipolarChannelCount;
		int m_nAuxChannelCount;
		double m_dSamplingRate;
		int m_nChunkSize;
		std::vector<std::string> m_psChannelLabels;
		std::vector<std::string> m_psEegChannelLabels;
		std::vector<std::string> m_psBipolarChannelLabels;
		std::vector<std::string> m_psAuxChannelLabels;
		bool m_bUnsampledMarkers;
		bool m_bSampledMarkersEEG;
		bool m_bUseACC;
		bool m_bUseSampleCounter;
		bool m_bIsSTEInDefault;
		bool m_bUseSim;
	};

	int m_nEegChannelCount;
	std::vector<int> m_pnUsableChannelsByDevice;
	bool m_bOverwrite;
	bool m_bOverrideAutoUpdate;
	LiveAmp m_LiveAmp;
	std::vector<std::string> m_psLiveAmpSns;
	std::unique_ptr<std::thread>  m_ptReaderThread;
	Ui::MainWindow* ui;
	bool m_bUseSimulators;
	bool m_bStop;
	t_TriggerOutputMode m_TriggerOutputMode;


	bool CheckConfiguration();
	void UpdateChannelCounters(int n);
	void WaitMessage();
	void UpdateChannelLabels(void);
	void ReadThread(t_AmpConfiguration ampConfiguration);
	void LoadConfig(const QString& filename);
	void SaveConfig(const QString& filename);
};

#endif // MAINWINDOW_H
