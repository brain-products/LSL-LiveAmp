#ifndef LiveAmp_H
#define LiveAmp_H

// Header file for higher level LiveAmp connection functions

// LiveAmp API
#include "Amplifier_LIB.h"
#include <vector>
#include <string>
#include <stdexcept>

class LiveAmp {

private:

	int m_nConnectedDevices;
	HANDLE m_Handle;
	std::string m_sSerialNumber;       
	int m_nAvailableChannels;         
	int m_nAvailableModules;            
	bool m_bHasSTE;                   
	int m_nSTEIdx;                     
	float m_fSamplingRate;
	int m_nRecordingMode;             

	// set during configuration
	typedef struct _channelInfo {
		int m_nDataType;
		float m_fResolution;
		int m_nChannelType;
		float m_fGain;
	}t_channelInfo;
	t_channelInfo m_pChannelInfo[100];
	
	std::vector<int> m_pnEegIndices;
	std::vector<int> m_pnAuxIndices;
	std::vector<int> m_pnAccIndices;
	std::vector<int> m_pnTrigIndices;

	int m_nSampleSize;
	int m_nEnabledChannelCnt;
	int m_nSampleCounterIdxInPush;
	bool m_bIsClosed;
	bool m_bIs64;
	bool m_bUseSampleCounter;
	bool m_bWasEnumerated;

public:

	LiveAmp(void) { m_bWasEnumerated = false; m_nConnectedDevices = -1; }
	~LiveAmp() { ; }
	
	// get the serial numbers and channel counts of all available liveamps
	void enumerate(std::vector<std::pair<std::string, int>> &ampData, bool useSim=false);

	void Error(const std::string& sError, int nErrorNum);
	//
	int Setup(std::string serialNumberIn, float samplingRateIn = 500, bool bUseSampleCounter = false, bool useSim = false, int recordingModeIn = RM_NORMAL);

	// close live amp device
	void close();

	// enable requested channels: for now acc and aux are all or nothing, triggers are always on, and eeg channels can be selected
	void enableChannels(const std::vector<int>& eegIndicesIn, const std::vector<int>& auxIndicesIn, bool accEnable);

	// activate the configured device with enabled channels
	void startAcquisition(void);

	// activate the configured device with enabled channels
	void stopAcquisition(void);

	// get data from device
	int64_t pullAmpData(BYTE* buffer, int bufferSize);

	// push it into a vector TODO: make this a template to support any buffer type
	void pushAmpData(BYTE* buffer, int bufferSize, int64_t samplesRead, std::vector<std::vector<float>> &outData);
	
	// set the output trigger output mode
	void setOutTriggerMode(t_TriggerOutputMode mode, int syncPin, int freq, int pulseWidth);

	// public data access methods 	
	inline float             getSamplingRate(void){return m_fSamplingRate;}
	inline HANDLE            getHandle(void) { if (m_Handle == NULL)return NULL;else return m_Handle; }
	inline std::string&      getSerialNumber(void){return m_sSerialNumber;}
	inline int               getAvailableChannels(void){return m_nAvailableChannels;}
	inline int               getRecordingMode(void){return m_nRecordingMode;}
	inline std::vector<int>& getEEGIndices(void){return m_pnEegIndices;}
	inline std::vector<int>& getAuxIndices(void){return m_pnAuxIndices;}
	inline std::vector<int>& getAccIndices(void){return m_pnAccIndices;}
	inline std::vector<int>& getTrigIndices(void){return m_pnTrigIndices;}
	inline int               getEnabledChannelCnt(void){return m_nEnabledChannelCnt;}
	inline int               getSampleSize(void){return m_nSampleSize;}
	inline bool				 isClosed(void){return m_bIsClosed;}
	inline bool              hasSTE(void) { return m_bHasSTE; }
	inline bool              is64(void) { return m_bIs64; }
	inline void              setUseSampleCounter(bool bUseSampleCounter) { m_bUseSampleCounter = bUseSampleCounter; }
};

#endif //LiveAmp_H