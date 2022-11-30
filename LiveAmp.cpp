#include "LiveAmp.h"

void LiveAmp::Error(const std::string& sError, int nErrorNum)
{
	std::string sErrorFromAmp;
	std::string sFullError = sError;
	switch (nErrorNum)
	{
	case -1:
		sErrorFromAmp = "AMP_ERR_FAIL";
		break;
		// TODO:
	case -2:
		sErrorFromAmp = "AMP_ERR_PARAM";
		break;
	case -3:
		sErrorFromAmp = "AMP_ERR_VERSION";
		break;
	case -4:
		sErrorFromAmp = "AMP_ERR_MEMORY";
		break;
	case -5:
		sErrorFromAmp = "AMP_ERR_BUSY";
		break;
	case -6:
		sErrorFromAmp = "AMP_ERR_NODEVICE";
		break;
	case -7:
		sErrorFromAmp = "AMP_ERR_NOSUPPORT";
		break;
	case -8:
		sErrorFromAmp = "AMP_ERR_EXCEPTION";
		break;
	case -9:
		sErrorFromAmp = "AMP_ERR_FWVERSION";
		break;
	case -10:
		sErrorFromAmp = "AMP_ERR_TIMEOUT";
		break;
	case -11:
		sErrorFromAmp = "AMP_ERR_BUFFERSIZE";
		break;

	case -101:
		sErrorFromAmp = "IF_ERR_FAIL";
		break;
	case -102:
		sErrorFromAmp = "IF_ERR_BT_SERVICE";
		break;
	case -103:
		sErrorFromAmp = "IF_ERR_MEMORY";
		break;
	case -104:
		sErrorFromAmp = "IF_ERR_NODEVICE";
		break;
	case -105:
		sErrorFromAmp = "IF_ERR_CONNECT";
		break;
	case -106:
		sErrorFromAmp = "IF_ERR_DISCONNECTED";
		break;
	case -107:
		sErrorFromAmp = "IF_ERR_TIMEOUT";
		break;
	case -108:
		sErrorFromAmp = "IF_ERR_ALREADYOPEN";
		break;
	case -109:
		sErrorFromAmp = "IF_ERR_PARAMETER";
		break;
	case -110:
		sErrorFromAmp = "IF_ERR_ATCOMMAND";
		break;

	case -200:
		sErrorFromAmp = "DEVICE_ERR_BASE";
		break;
	case -201:
		sErrorFromAmp = "DEVICE_ERR_FAIL";
		break;
	case -202:
		sErrorFromAmp = "DEVICE_ERR_PARAM";
		break;
	case -203:
		sErrorFromAmp = "DEVICE_ERR_VERSION";
		break;
	case -204:
		sErrorFromAmp = "DEVICE_ERR_MEMORY";
		break;
	case -205:
		sErrorFromAmp = "DEVICE_ERR_BUSY";
		break;
	case -206:
		sErrorFromAmp = "DEVICE_ERR_SDWRITE";
		break;
	case -207:
		sErrorFromAmp = "DEVICE_ERR_SDREAD";
		break;
	case -208:
		sErrorFromAmp = "DEVICE_ERR_NOSD";
		break;
	}
	sFullError.append(sErrorFromAmp);
	throw std::runtime_error(sFullError);
}

LiveAmp::LiveAmp(std::string sSerialNumber, float fSamplingRate, bool bUseSampleCounter, int nRecordingMode) {

	m_bHasSTE = false;
	m_bIs64 = false;
	m_bUseSampleCounter = bUseSampleCounter;

	for (int i = 0; i < 50; i++)
	{
		int nResult;

		HANDLE handle = NULL;
		nResult = ampOpenDevice(i, &handle);

		if (nResult == AMP_OK)
		{

			m_Handle = handle;
			char sVar[20];
			sVar[19] = 0;
			nResult = ampGetProperty(handle, PG_DEVICE, i, DPROP_CHR_SerialNumber, sVar, sizeof(sVar));

			// got a hit!
			if (!(strcmp(sVar, sSerialNumber.c_str()))) {

				// set the device mode to recording
				nResult = ampSetProperty(m_Handle, PG_DEVICE, 0, DPROP_I32_RecordingMode, &nRecordingMode, sizeof(nRecordingMode));
				if (nResult != AMP_OK)
					Error("Error setting acquisition mode, error code:  ", nResult);
				m_nRecordingMode = nRecordingMode;
				m_sSerialNumber = std::string(sVar);
				nResult = ampGetProperty(m_Handle, PG_DEVICE, 0, DPROP_I32_AvailableChannels, &m_nAvailableChannels, sizeof(m_nAvailableChannels));
				if (nResult != AMP_OK)
					Error("Error getting available channel count, error code: ", nResult);

				nResult = ampGetProperty(m_Handle, PG_DEVICE, 0, DPROP_I32_AvailableModules, &m_nAvailableModules, sizeof(m_nAvailableModules));
				if (nResult != AMP_OK)
					Error("Error getting available module channel count, error code:  ", nResult);
				char sModName[100]; sModName[99] = 0;
				for (int n = 0; n < m_nAvailableModules; n++)
				{
					nResult = ampGetProperty(m_Handle, PG_MODULE, n, MPROP_CHR_Type, &sModName, sizeof(sModName));
					if (!strcmp(sModName, "STE"))
					{
						m_bHasSTE = true;
						m_nSTEIdx = n;
					}
				}

				m_bIs64 = false;
				char sType[100]; sType[99] = 0;
				nResult = ampGetProperty(m_Handle, PG_DEVICE, 0, DPROP_CHR_Type, &sType, sizeof(sType));
				if (nResult != AMP_OK)
					Error("Error getting device name, error code: ", nResult);

				if (!(strcmp("LiveAmp64", sType)))
					m_bIs64 = true;

				nResult = ampSetProperty(m_Handle, PG_DEVICE, 0, DPROP_F32_BaseSampleRate,
					&fSamplingRate, sizeof(fSamplingRate));
				if (nResult != AMP_OK)
					Error("Error setting sampling rate, error code: ", nResult);
				m_fSamplingRate = fSamplingRate;
				break;
			}
		}
	}
	m_bIsClosed = false;
}

int LiveAmp::enumerate(std::vector<std::pair<std::string, int>>& ampData, bool useSim) {

	int nRes;
	char HWI[20];
	int nConnectedDevices;
	if (!ampData.empty()) {
		throw std::runtime_error("Input ampData vector isn't empty");
		return -1;
	}
	if(useSim)
		strcpy_s(HWI, "SIM");
	else
		strcpy_s(HWI, "ANY");

	nRes = ampEnumerateDevices(HWI, sizeof(HWI), "", 0);
	nConnectedDevices = nRes;
	if (nRes <= 0)
		throw std::runtime_error("No LiveAmp connected");
	else {
		for (int i = 0; i < nRes; i++)
		{
			int nResult;
			HANDLE handle = NULL;

			nResult = ampOpenDevice(i, &handle);
			if (nResult != AMP_OK) {
				std::string msg = "Cannot open device: ";
				msg.append(std::to_string(i));
				msg.append("  error= ");
				Error(msg, nResult);
			}

			char sVar[100]; sVar[99] = 0;
			nResult = ampGetProperty(handle, PG_DEVICE, i, DPROP_CHR_SerialNumber, sVar, sizeof(sVar));
			if (nResult != AMP_OK) {
				std::string msg = "Cannot get device serial number: ";
				msg.append(std::to_string(i));
				msg.append("  error= ");
				Error(msg, nResult);
			}

			int32_t nAvailableModules;
			//int32_t nAvailableChannels;
			nResult = ampGetProperty(handle, PG_DEVICE, 0, DPROP_I32_AvailableModules, &nAvailableModules, sizeof(nAvailableModules));
			int32_t nVar;
			char sModName[100]; sModName[99] = 0;
			int nTotalAvailableChannels = 0;
			for (int j = 0; j < nAvailableModules; j++)
			{
				nResult = ampGetProperty(handle, PG_MODULE, j, MPROP_I32_UseableChannels, &nVar, sizeof(nVar));
				if (nResult != AMP_OK) {
					std::string msg = "Cannot get device channel count: ";
					msg.append(std::to_string(j));
					msg.append("  error= ");
					Error(msg, nResult);
				}

				nResult = ampGetProperty(handle, PG_MODULE, j, MPROP_CHR_Type, &sModName, sizeof(sModName));
				if (strcmp(sModName, "STE"))
					nTotalAvailableChannels += nVar;
			}
			ampData.push_back(std::make_pair(std::string(sVar), nTotalAvailableChannels));

			nResult = ampCloseDevice(handle);
			if (nResult != AMP_OK) {
				std::string msg = "Cannot close device: ";
				msg.append("  error= ");
				Error(msg, nResult);
			}
		}
	}
	return nConnectedDevices;
}


void LiveAmp::close(void) {
	if (m_bIsClosed)return;
	int nResult = ampCloseDevice(m_Handle);
	if (nResult != AMP_OK) {
		std::string msg = "Cannot close device: ";
		msg.append(m_sSerialNumber.c_str());
		msg.append("  error= ");
		Error(msg, nResult);
	}
	m_bIsClosed = true;
}

void LiveAmp::enableChannels(const std::vector<int>& pnEegIndices, const std::vector<int>& pnAuxIndices, bool bAccEnable) {

	int nRes;
	int nType;
	int nEnable;
	int nBipType = CT_BIP;
	char sValue[20]; // for determining if the aux channel is an accelerometer or not
	char sUnit[256];
	int nMaxEEGNumber = 0;
	if (!m_pnEegIndices.empty())m_pnEegIndices.clear();
	if (!m_pnAuxIndices.empty())m_pnAuxIndices.clear();
	if (!m_pnAccIndices.empty())m_pnAccIndices.clear();
	if (!m_pnTrigIndices.empty())m_pnTrigIndices.clear();
	m_nEnabledChannelCnt = 0;

	if (m_nAvailableChannels == -1)
	{
		throw std::runtime_error((std::string("Invalid number of available channels on device ") + m_sSerialNumber).c_str());
		return;
	}
	nEnable = 0;
	// go through the available channels and enable them if they are chosen
	for (int i = 0; i < m_nAvailableChannels; i++)
	{
		nEnable = 0;
		nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_I32_Type, &nType, sizeof(nType));
		if (nRes != AMP_OK)
			throw std::runtime_error("Error getting property for channel type: error code:  " + std::to_string(nRes));
		nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_CHR_Unit, sUnit, sizeof(sUnit));
		if (nType == CT_EEG || nType == CT_BIP)
		{
			for (std::vector<int>::const_iterator it = pnEegIndices.begin(); it != pnEegIndices.end(); ++it)
			{
				if (*it == i)
				{
					nEnable = 1;
					if (nRes != AMP_OK)
						throw std::runtime_error("Error SetProperty enable for EEG channels, error: " + std::to_string(nRes));
					m_pnEegIndices.push_back(i);
					++m_nEnabledChannelCnt;

				}
			}
			nMaxEEGNumber++;
		}
		nRes = ampSetProperty(m_Handle, PG_CHANNEL, i, CPROP_B32_RecordingEnabled, &nEnable, sizeof(nEnable));
		nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_B32_RecordingEnabled, &nEnable, sizeof(nEnable));
		int nFoo = 0;
		nFoo = 1;
	}

	for (int i = 0; i < m_nAvailableChannels; i++)
	{
		nEnable = 0;
		nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_I32_Type, &nType, sizeof(nType));
		if (nRes != AMP_OK)
			Error("Error getting property for channel type: error code:  ", nRes);

		if (nType == CT_AUX)
		{
			nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_CHR_Function, &sValue, sizeof(sValue));

			if (nRes != AMP_OK)
				Error("Error GetProperty CPROP_CHR_Function error: ", nRes);

			// check that this aux channel is an acc channel
			if (sValue[0] == 'X' || sValue[0] == 'Y' || sValue[0] == 'Z' || sValue[0] == 'x' || sValue[0] == 'y' || sValue[0] == 'z')
			{
				if (bAccEnable)
				{
					nEnable = 1;
					nRes = ampSetProperty(m_Handle, PG_CHANNEL, i, CPROP_B32_RecordingEnabled, &nEnable, sizeof(nEnable));
					if (nRes != AMP_OK)
						Error("Error SetProperty enable for ACC channels, error: ", nRes);
					m_pnAccIndices.push_back(i);
					++m_nEnabledChannelCnt;
				}
			}

			else
			{
				for (std::vector<int>::const_iterator it = pnAuxIndices.begin(); it != pnAuxIndices.end(); ++it)
				{
					if (*it + nMaxEEGNumber == i)
					{
						nEnable = 1;
						nRes = ampSetProperty(m_Handle, PG_CHANNEL, i, CPROP_B32_RecordingEnabled, &nEnable, sizeof(nEnable));
						if (nRes != AMP_OK)
							Error("Error SetProperty enable for AUX channels, error: ", nRes);
						m_pnAuxIndices.push_back(i);
						++m_nEnabledChannelCnt;
					}
				}
			}
		}

		if (nType == CT_TRG)
		{
			nEnable = 1;
			nRes = ampSetProperty(m_Handle, PG_CHANNEL, i, CPROP_B32_RecordingEnabled, &nEnable, sizeof(nEnable));
			m_pnTrigIndices.push_back(i);
			++m_nEnabledChannelCnt;
		}
		if (nType == CT_DIG)
		{
			nEnable = 1;
			nRes = ampSetProperty(m_Handle, PG_CHANNEL, i, CPROP_B32_RecordingEnabled, &nEnable, sizeof(nEnable));
			++m_nEnabledChannelCnt;
		}
	}

	m_nSampleCounterIdxInPush = (int)(m_pnEegIndices.size() +
		m_pnAccIndices.size() +
		m_pnAuxIndices.size()) + 3;

	int nDataType;
	float fResolution;
	int nChannelType;
	float fGain;
	m_nSampleSize = 0;
	int nEnabled;

	nRes = ampStartAcquisition(m_Handle);
	m_pChannelInfo.clear();
	for (int i = 0; i < m_nAvailableChannels; i++)
	{
		nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_B32_RecordingEnabled, &nEnabled, sizeof(nEnabled));

		if (nEnabled)
		{
			t_channelInfo channelInfo;
			nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_I32_DataType, &nDataType, sizeof(nDataType));
			channelInfo.m_nDataType = nDataType;
			nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_F32_Resolution, &fResolution, sizeof(fResolution));
			channelInfo.m_fResolution = fResolution;
			nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_I32_Type, &nChannelType, sizeof(nChannelType));
			channelInfo.m_nChannelType = nChannelType;
			nRes = ampGetProperty(m_Handle, PG_CHANNEL, i, CPROP_F32_Gain, &fGain, sizeof(fGain));
			channelInfo.m_fGain = fGain;
			m_pChannelInfo.push_back(channelInfo);
			switch (nDataType)
			{
			case DT_INT16:
			case DT_UINT16:
			{
				m_nSampleSize += 2;
			}
			break;
			case DT_INT32:
			case DT_UINT32:
			case DT_FLOAT32:
			{
				m_nSampleSize += 4;

			}
			break;
			case DT_INT64:
			case DT_UINT64:
			case DT_FLOAT64:
			{
				m_nSampleSize += 8;
			}
			break;
			default:
				break;
			}
		}
	}
	// add the sample counter size
	m_nSampleSize += 8;
	nRes = ampStopAcquisition(m_Handle);
	if (m_pChannelInfo.size() != m_nEnabledChannelCnt)
		throw std::runtime_error((std::string("Error: Enabled channel counter mismatch in device ") + m_sSerialNumber).c_str());
}

void LiveAmp::startAcquisition(void) {

	int nRes = ampStartAcquisition(m_Handle);
	if (nRes != AMP_OK)
		Error("Error starting acquisition, error code:  ", nRes);

}

void LiveAmp::stopAcquisition(void) {

	int nRes = ampStopAcquisition(m_Handle);
	if (nRes != AMP_OK)
		Error("Error stopping acquisition, error code:  ", nRes);

}

int64_t LiveAmp::pullAmpData(BYTE* pBuffer, int nBufferSize) {
	int64_t nSamplesRead = ampGetData(m_Handle, pBuffer, nBufferSize, 0);
	return nSamplesRead;
}

void LiveAmp::pushAmpData(BYTE* pBuffer, int nBufferSize, int64_t nSamplesRead, std::vector<std::vector<float>>& pfOutData)
{
	uint64_t nSampleCount;

	int nOffset = 0;
	float fSample = 0;
	int nTriggTmp = 0;
	int isSecondBit = 0;

	int64_t nNumSamples = nSamplesRead / m_nSampleSize;

	std::vector<float> pfTmpData;

	for (int s = 0; s < nNumSamples; s++)
	{
		nOffset = 0;
		nSampleCount = *(uint64_t*)&pBuffer[s * m_nSampleSize + nOffset];
		nOffset += 8; // sample counter offset 

		pfTmpData.resize(m_nEnabledChannelCnt);

		for (int i = 0; i < m_nEnabledChannelCnt; i++)
		{
			switch (m_pChannelInfo[i].m_nDataType)
			{
			case DT_INT16:
			{
				int16_t tmp = *(int16_t*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				nOffset += 2;
				break;
			}
			case DT_UINT16:
			{
				// note: in the case of liveamp, this floating point sample
				// will need to be converted back to an integer and &ed with the
				// second bit for the true value of the trigger
				uint16_t tmp = *(uint16_t*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				if (((int)fSample >> 8) == 254)
					tmp = tmp;
				nOffset += 2;
				break;
			}
			case DT_INT32:
			{
				int32_t tmp = *(int32_t*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				nOffset += 4;
				break;
			}
			case DT_UINT32:
			{
				uint32_t tmp = *(uint32_t*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				nOffset += 4;
				break;
			}
			case DT_FLOAT32:
			{
				float tmp = *(float*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				nOffset += 4;
				break;
			}
			case DT_INT64:
			{
				int64_t tmp = *(int64_t*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				nOffset += 8;
				break;
			}
			case DT_UINT64:
			{
				uint64_t tmp = *(uint64_t*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				nOffset += 8;
				break;
			}
			case DT_FLOAT64:
			{

				float tmp = *(float*)&pBuffer[s * m_nSampleSize + nOffset];
				fSample = (float)tmp * m_pChannelInfo[i].m_fResolution;
				nOffset += 8;
				break;
			}
			default:
				break;
			}

			pfTmpData[i] = fSample;
		}
		if (m_bUseSampleCounter)
			pfTmpData[m_nSampleCounterIdxInPush] = (float)nSampleCount;
		pfOutData.push_back(pfTmpData);
	}
}

void LiveAmp::setOutTriggerMode(t_TriggerOutputMode triggerMode, int nSyncPin, int nFreq, int nPulseWidth)
{
	if (!m_bHasSTE)return;
	int nPer = (int)m_fSamplingRate / nFreq;
	int res = ampSetProperty(m_Handle, PG_MODULE, m_nSTEIdx, MPROP_I32_TriggerOutMode, &triggerMode, sizeof(triggerMode));
	res = ampSetProperty(m_Handle, PG_MODULE, m_nSTEIdx, MPROP_I32_TriggerSyncPin, &nSyncPin, sizeof(nSyncPin));
	res = ampSetProperty(m_Handle, PG_MODULE, m_nSTEIdx, MPROP_I32_TriggerSyncPeriod, &nPer, sizeof(nPer));
	res = ampSetProperty(m_Handle, PG_MODULE, m_nSTEIdx, MPROP_I32_TriggerSyncWidth, &nPulseWidth, sizeof(nPulseWidth));
	if (res != AMP_OK)
		Error("Error setting trigger output mode, error code:  ", res);
}

