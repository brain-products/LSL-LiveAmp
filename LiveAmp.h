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

	HANDLE m_Handle = nullptr;
	std::string m_sSerialNumber;       
	int m_nAvailableChannels = -1;         
	int m_nAvailableModules = -1;            
	bool m_bHasSTE = false;                   
	int m_nSTEIdx = -1;                     
	float m_fSamplingRate = -1.0;
	int m_nRecordingMode = -1;             

	// set during configuration
	typedef struct _channelInfo {
		int m_nDataType;
		float m_fResolution;
		int m_nChannelType;
		float m_fGain;
	}t_channelInfo;

	std::vector<t_channelInfo> m_pChannelInfo;	
	std::vector<int> m_pnEegIndices;
	std::vector<int> m_pnAuxIndices;
	std::vector<int> m_pnAccIndices;
	std::vector<int> m_pnTrigIndices;

	int m_nSampleSize = -1;
	int m_nEnabledChannelCnt = -1;
	int m_nSampleCounterIdxInPush = -1;
	bool m_bIsClosed = false;
	bool m_bIs64 = false;
	bool m_bUseSampleCounter = false;
	bool m_bWasEnumerated = false;

public:

	LiveAmp(std::string serialNumberIn, float samplingRateIn = 500, bool bUseSampleCounter = false, int recordingModeIn = RM_NORMAL);
	~LiveAmp() { close(); }
	
	// get the serial numbers and channel counts of all available liveamps
	static int enumerate(std::vector<std::pair<std::string, int>> &ampData, bool useSim = false);

	static void Error(const std::string& sError, int nErrorNum);
	
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