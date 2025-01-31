/****************************************************************************
 *
 * Copyright (c) 2012 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include "ioctl.h"
#include "debug.h"
#include "mlme.h"
#include "mgt.h"
#include "cac.h"
#include "hip.h"
#include "netif.h"
#include <net/netlink.h>
#include <linux/netdevice.h>
#include <linux/ieee80211.h>
#include "mib.h"
#include <scsc/scsc_mx.h>
#include "dev.h"
#include "oxygen_ioctl.h"

#define CMD_RXFILTERADD         "RXFILTER-ADD"
#define CMD_RXFILTERREMOVE              "RXFILTER-REMOVE"
#define CMD_RXFILTERSTART               "RXFILTER-START"
#define CMD_RXFILTERSTOP                "RXFILTER-STOP"
#define CMD_SETCOUNTRYREV               "SETCOUNTRYREV"
#define CMD_GETCOUNTRYREV               "GETCOUNTRYREV"
#define CMD_SETROAMTRIGGER              "SETROAMTRIGGER"
#define CMD_GETROAMTRIGGER              "GETROAMTRIGGER"
#define CMD_SETSUSPENDMODE              "SETSUSPENDMODE"
#define CMD_SETROAMDELTA                "SETROAMDELTA"
#define CMD_GETROAMDELTA                "GETROAMDELTA"
#define CMD_SETROAMSCANPERIOD           "SETROAMSCANPERIOD"
#define CMD_GETROAMSCANPERIOD           "GETROAMSCANPERIOD"
#define CMD_SETFULLROAMSCANPERIOD               "SETFULLROAMSCANPERIOD"
#define CMD_GETFULLROAMSCANPERIOD               "GETFULLROAMSCANPERIOD"
#define CMD_SETSCANCHANNELTIME          "SETSCANCHANNELTIME"
#define CMD_GETSCANCHANNELTIME          "GETSCANCHANNELTIME"
#define CMD_SETROAMSCANCHANNELMINTIME           "SETROAMSCANCHANNELMINTIME"
#define CMD_GETROAMSCANCHANNELMINTIME           "GETROAMSCANCHANNELMINTIME"
#define CMD_SETSCANNPROBES              "SETSCANNPROBES"
#define CMD_GETSCANNPROBES              "GETSCANNPROBES"
#define CMD_SETROAMMODE         "SETROAMMODE"
#define CMD_GETROAMMODE         "GETROAMMODE"
#define CMD_SETCCXMODE          "SETCCXMODE"
#define CMD_GETCCXMODE          "GETCCXMODE"
#define CMD_SETROAMINTRABAND            "SETROAMINTRABAND"
#define CMD_GETROAMINTRABAND            "GETROAMINTRABAND"
#define CMD_SETROAMBAND         "SETROAMBAND"
#define CMD_GETROAMBAND         "GETROAMBAND"
#define CMD_SETROAMSCANCONTROL          "SETROAMSCANCONTROL"
#define CMD_GETROAMSCANCONTROL          "GETROAMSCANCONTROL"
#define CMD_SETSCANHOMETIME             "SETSCANHOMETIME"
#define CMD_GETSCANHOMETIME             "GETSCANHOMETIME"
#define CMD_SETSCANHOMEAWAYTIME         "SETSCANHOMEAWAYTIME"
#define CMD_GETSCANHOMEAWAYTIME         "GETSCANHOMEAWAYTIME"
#define CMD_SETOKCMODE          "SETOKCMODE"
#define CMD_GETOKCMODE          "GETOKCMODE"
#define CMD_SETWESMODE          "SETWESMODE"
#define CMD_GETWESMODE          "GETWESMODE"
#define CMD_SET_PMK             "SET_PMK"
#define CMD_HAPD_GET_CHANNEL			"HAPD_GET_CHANNEL"
#define CMD_SET_SAP_CHANNEL_LIST                "SET_SAP_CHANNEL_LIST"
#define CMD_REASSOC             "REASSOC"
#define CMD_SETROAMSCANCHANNELS         "SETROAMSCANCHANNELS"
#define CMD_GETROAMSCANCHANNELS         "GETROAMSCANCHANNELS"
#define CMD_SENDACTIONFRAME             "SENDACTIONFRAME"
#define CMD_HAPD_MAX_NUM_STA            "HAPD_MAX_NUM_STA"
#define CMD_COUNTRY            "COUNTRY"
#define CMD_SEND_GK                               "SEND_GK"
#ifdef CONFIG_SCSC_WLAN_OXYGEN_ENABLE
#define CMD_SETIBSSBEACONOUIDATA "SETIBSSBEACONOUIDATA"
#define CMD_GETIBSSPEERINFOALL "GETIBSSPEERINFOALL"
#define CMD_GETIBSSPEERINFO "GETIBSSPEERINFO"
#define CMD_SETIBSSAMPDU "SETIBSSAMPDU"
#define CMD_SETIBSSANTENNAMODE "SETIBSSANTENNAMODE"
#define CMD_SETRMCENABLE "SETRMCENABLE"
#define CMD_SETRMCTXRATE "SETRMCTXRATE"
#define CMD_SETRMCACTIONPERIOD "SETRMCACTIONPERIOD"
#define CMD_SETRMCLEADER "SETRMCLEADER"
#define CMD_SETIBSSTXFAILEVENT "SETIBSSTXFAILEVENT"
#define CMD_SETIBSSROUTETABLE "SETIBSSROUTETABLE"
#endif
#define CMD_SETAPP2PWPSIE "SET_AP_P2P_WPS_IE"
#define CMD_P2PSETPS "P2P_SET_PS"
#define CMD_P2PSETNOA "P2P_SET_NOA"
#define CMD_P2PECSA "P2P_ECSA"
#define CMD_P2PLOSTART "P2P_LO_START"
#define CMD_P2PLOSTOP "P2P_LO_STOP"
#define CMD_TDLSCHANNELSWITCH  "TDLS_CHANNEL_SWITCH"
#define CMD_SETROAMOFFLOAD     "SETROAMOFFLOAD"
#define CMD_SETROAMOFFLAPLIST  "SETROAMOFFLAPLIST"
#define CMD_GETDFSSCANMODE     "GETDFSSCANMODE"

#define CMD_SETBAND "SETBAND"
#define CMD_GETBAND "GETBAND"

/* Known commands from MCD framework for which no handlers */
#define CMD_AMPDU_MPDU "AMPDU_MPDU"
#define CMD_BTCOEXMODE "BTCOEXMODE"
#define CMD_BTCOEXSCAN_START "BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP "BTCOEXSCAN-STOP"
#define CMD_CHANGE_RL "CHANGE_RL"
#define CMD_INTERFACE_CREATE "INTERFACE_CREATE"
#define CMD_INTERFACE_DELETE "INTERFACE_DELETE"
#define CMD_LTECOEX "LTECOEX"
#define CMD_MIRACAST "MIRACAST"
#define CMD_RESTORE_RL "RESTORE_RL"
#define CMD_RPSMODE "RPSMODE"
#define CMD_SETCCXMODE "SETCCXMODE"
#define CMD_SETDFSSCANMODE "SETDFSSCANMODE"
#define CMD_SET_FCC_CHANNEL "SET_FCC_CHANNEL"
#define CMD_SETJOINPREFER "SETJOINPREFER"
#define CMD_SETSINGLEANT "SETSINGLEANT"
#define CMD_SET_TX_POWER_CALLING "SET_TX_POWER_CALLING"

#define CMD_DRIVERDEBUGDUMP "DEBUG_DUMP"
#define CMD_TESTFORCEHANG "SLSI_TEST_FORCE_HANG"
#define CMD_GETREGULATORY "GETREGULATORY"

#define ROAMOFFLAPLIST_MIN 1
#define ROAMOFFLAPLIST_MAX 100

static int slsi_parse_hex(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

static void slsi_machexstring_to_macarray(char *mac_str, u8 *mac_arr)
{
	mac_arr[0] = slsi_parse_hex(mac_str[0]) << 4 | slsi_parse_hex(mac_str[1]);
	mac_arr[1] = slsi_parse_hex(mac_str[3]) << 4 | slsi_parse_hex(mac_str[4]);
	mac_arr[2] = slsi_parse_hex(mac_str[6]) << 4 | slsi_parse_hex(mac_str[7]);
	mac_arr[3] = slsi_parse_hex(mac_str[9]) << 4 | slsi_parse_hex(mac_str[10]);
	mac_arr[4] = slsi_parse_hex(mac_str[12]) << 4 | slsi_parse_hex(mac_str[13]);
	mac_arr[5] = slsi_parse_hex(mac_str[15]) << 4 | slsi_parse_hex(mac_str[16]);
}

static ssize_t slsi_set_suspend_mode(struct net_device *dev, char *command)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               user_suspend_mode = sdev->device_config.user_suspend_mode;
	int               ret = 0;

	sdev->device_config.user_suspend_mode = *(command + strlen(CMD_SETSUSPENDMODE) + 1) - '0';

	if (user_suspend_mode != sdev->device_config.user_suspend_mode)
		(void)slsi_update_suspend_mode(sdev);

	return ret;
}

static ssize_t slsi_set_p2p_oppps(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif;
	struct slsi_dev   *sdev;

	u8                   *p2p_oppps_param = NULL;
	int                  offset = 0;
	struct slsi_mib_data mib_data = { 0, NULL };
	int                  error = 0;
	unsigned int         ct_param;
	int                  result = 0;

	SLSI_NET_DBG2(dev, SLSI_CFG80211, "Function entry\n");

	p2p_oppps_param = command + strlen(CMD_P2PSETPS) + 1;
	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	/* The NOA param shall be added only after P2P-VIF is active */
	if ((!ndev_vif->activated) || (ndev_vif->iftype != NL80211_IFTYPE_P2P_GO)) {
		SLSI_ERR_NODEV("P2P GO vif not activated\n");
		result = -EINVAL;
		goto exit;
	}

	sdev = ndev_vif->sdev;
	offset = slsi_str_to_int(&p2p_oppps_param[0], &ct_param);
	if (!offset) {
		SLSI_ERR(sdev, "ct_param: failed to read a numeric value\n");
		result = -EINVAL;
		goto exit;
	}

	if (ct_param == 1) {
		SLSI_WARN(sdev, "p2p oppps = %d, Ignore this command as p2p oppps already active\n", ct_param);
		goto exit;
	} else if (ct_param < (unsigned int)ndev_vif->ap.beacon_interval) {
		SLSI_DBG1(sdev, SLSI_CFG80211, "p2p ct window = %d\n", ct_param);
		error = slsi_mib_encode_uint(&mib_data, SLSI_PSID_UNIFI_CT_WINDOW, ct_param, 0);
	} else {
		SLSI_DBG1(sdev, SLSI_CFG80211, "p2p ct window = %d is out of range for beacon interval(%d)\n", ct_param, ndev_vif->ap.beacon_interval);
		goto exit;
	}
	if ((error != SLSI_MIB_STATUS_SUCCESS) || (mib_data.dataLength == 0)) {
		result = -ENOMEM;
		goto exit;
	}

	result = slsi_mlme_set(sdev, dev, mib_data.data, mib_data.dataLength);
	kfree(mib_data.data);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return result;
}

static ssize_t slsi_p2p_set_noa_params(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif;
	struct slsi_dev   *sdev;

	int                  result = 0;
	u8                   *noa_params = NULL;
	int                  offset = 0;
	int                  readbyte = 0;
	struct slsi_mib_data mib_data = { 0, NULL };
	int                  error = 0;
	unsigned int         noa_count;
	unsigned int         start_time;
	unsigned int         duration;

	SLSI_NET_DBG4(dev, SLSI_NETDEV, "Function entry\n");

	noa_params = command + strlen(CMD_P2PSETNOA) + 1;
	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	/* The NOA param shall be added only after P2P-VIF is active */
	if ((!ndev_vif->activated) || (ndev_vif->iftype != NL80211_IFTYPE_P2P_GO)) {
		SLSI_ERR_NODEV("P2P GO vif not activated\n");
		result = -EINVAL;
		goto exit;
	}

	sdev = ndev_vif->sdev;
	readbyte = slsi_str_to_int(&noa_params[offset], &noa_count);
	if (!readbyte) {
		SLSI_ERR(sdev, "noa_count: failed to read a numeric value\n");
		result = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	readbyte = slsi_str_to_int(&noa_params[offset], &start_time);
	if (!readbyte) {
		SLSI_ERR(sdev, "start_time: failed to read a numeric value\n");
		result = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	readbyte = slsi_str_to_int(&noa_params[offset], &duration);
	if (!readbyte) {
		SLSI_ERR(sdev, "duration: failed to read a numeric value, at offset(%d)\n", offset);
		result = -EINVAL;
		goto exit;
	}

	SLSI_DBG1(sdev, SLSI_CFG80211, "p2p noa_params (count,starttime,duration)= (%d,%d,%d)\n", noa_count, start_time, duration);

	error = slsi_mib_encode_uint(&mib_data, SLSI_PSID_UNIFI_NOA_COUNT, noa_count, 0);
	if (error != SLSI_MIB_STATUS_SUCCESS) {
		result = -ENOMEM;
		goto exit;
	}
	if (noa_count) {
		error = slsi_mib_encode_uint(&mib_data, SLSI_PSID_UNIFI_NOA_START_OFFSET, start_time, 0);
		if (error != SLSI_MIB_STATUS_SUCCESS) {
			result = -ENOMEM;
			goto mibexit;
		}
		error = slsi_mib_encode_uint(&mib_data, SLSI_PSID_UNIFI_NOA_DURATION, duration, 0);
		if (error != SLSI_MIB_STATUS_SUCCESS) {
			result = -ENOMEM;
			goto mibexit;
		}
	}
	result = slsi_mlme_set(sdev, dev, mib_data.data, mib_data.dataLength);
mibexit:
	kfree(mib_data.data);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return result;
}

static ssize_t slsi_p2p_ecsa(struct net_device *dev, char *command)
{
	struct netdev_vif *ndev_vif;
	struct slsi_dev   *sdev;

	int                  result = 0;
	u8                   *ecsa_params = NULL;
	int                  offset = 0;
	int                  readbyte = 0;
	unsigned int         channel;
	unsigned int         bandwidth;
	u16 center_freq = 0;
	u16 chan_info = 0;
	struct cfg80211_chan_def chandef;
	enum ieee80211_band band;
	enum nl80211_channel_type chan_type = NL80211_CHAN_NO_HT;

	SLSI_NET_DBG4(dev, SLSI_NETDEV, "Function entry\n");

	ecsa_params = command + strlen(CMD_P2PECSA) + 1;
	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	/* The ECSA param shall be added only after P2P-GO-VIF is active */
	if ((!ndev_vif->activated) || (ndev_vif->iftype != NL80211_IFTYPE_P2P_GO)) {
		SLSI_ERR_NODEV("P2P GO vif not activated\n");
		result = -EINVAL;
		goto exit;
	}

	sdev = ndev_vif->sdev;
	readbyte = slsi_str_to_int(&ecsa_params[offset], &channel);
	if (!readbyte) {
		SLSI_ERR(sdev, "channel: failed to read a numeric value\n");
		result = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	readbyte = slsi_str_to_int(&ecsa_params[offset], &bandwidth);
	if (!readbyte) {
		SLSI_ERR(sdev, "bandwidth: failed to read a numeric value\n");
		result = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	SLSI_DBG1(sdev, SLSI_CFG80211, "p2p ecsa_params (channel and bandwidth)= (%d,%d)\n", channel, bandwidth);

	band = (channel <= 14) ? IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ;
	center_freq = ieee80211_channel_to_frequency(channel, band);
	SLSI_DBG1(sdev, SLSI_CFG80211, "p2p ecsa_params (center_freq)= (%d)\n", center_freq);
	chandef.chan = ieee80211_get_channel(sdev->wiphy, center_freq);
	chandef.width = (band  == IEEE80211_BAND_2GHZ) ? NL80211_CHAN_WIDTH_20_NOHT : NL80211_CHAN_WIDTH_80;

#ifndef SSB_4963_FIXED
	/* Default HT40 configuration */
#ifdef SCSC_WLAN_ENABLE_5GHZ
	SLSI_DBG1(sdev, SLSI_CFG80211, "p2p ecsa_params Default HT40\n");
	if (bandwidth == 80) {
		chandef.width = NL80211_CHAN_WIDTH_40;
		if (channel == 36 || channel == 44 || channel == 149 || channel == 157)
			chan_type = NL80211_CHAN_HT40PLUS;
		else
			chan_type = NL80211_CHAN_HT40MINUS;
	}
#endif
#endif
	SLSI_DBG1(sdev, SLSI_CFG80211, "p2p ecsa_params chan_type = %d\n", chan_type);
	cfg80211_chandef_create(&chandef, chandef.chan, chan_type);
	chan_info = slsi_get_chann_info(sdev, &chandef);
	if (bandwidth != 20)
		center_freq = slsi_get_center_freq1(sdev, chan_info, center_freq);
	result = slsi_mlme_channel_switch(sdev, dev, center_freq, chan_info);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return result;
}

static ssize_t slsi_p2p_go_vendor_ies_write(struct slsi_dev *sdev, struct net_device *dev, u8 *ie, size_t ie_len, u16 purpose)
{
	u8                *go_vendor_ie = NULL;
	int               result = 0;
	struct netdev_vif *ndev_vif;

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	/* During AP start before mlme_start_req, supplicant calls set_ap_wps_ie() to send the vendor IEs for each
	 * beacon, probe response and association response. As we get all of them in mlme_start_req, ignoring the
	 * same which comes before adding GO VIF
	 */
	if (!ndev_vif->activated) {
		SLSI_DBG1(sdev, SLSI_CFG80211, "P2P GO vif not activated\n");
		result = 0;
		goto exit;
	}
	if (ndev_vif->iftype != NL80211_IFTYPE_P2P_GO) {
		SLSI_ERR(sdev, "No P2P interface present\n");
		result = -EINVAL;
		goto exit;
	}

	go_vendor_ie = kmalloc(ie_len, GFP_KERNEL);
	if (go_vendor_ie == NULL) {
		SLSI_ERR(sdev, "kmalloc failed\n");
		result = -ENOMEM;
		goto exit;
		}
	memcpy(go_vendor_ie, ie, ie_len);

	slsi_clear_cached_ies(&ndev_vif->ap.add_info_ies, &ndev_vif->ap.add_info_ies_len);
	result = slsi_ap_prepare_add_info_ies(ndev_vif, go_vendor_ie, ie_len);

	if (result == 0)
		result = slsi_mlme_add_info_elements(sdev, dev, purpose, ndev_vif->ap.add_info_ies, ndev_vif->ap.add_info_ies_len);

	slsi_clear_cached_ies(&ndev_vif->ap.add_info_ies, &ndev_vif->ap.add_info_ies_len);
	kfree(go_vendor_ie);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return result;
}

static ssize_t slsi_set_ap_p2p_wps_ie(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int                  readbyte = 0;
	int                  offset = 0;
	int                  result = 0;
	enum if_type {
		IF_TYPE_NONE,
		IF_TYPE_P2P_DEVICE,
		IF_TYPE_P2P_INTERFACE
	} iftype = IF_TYPE_NONE;
	enum frame_type {
		FRAME_TYPE_NONE,
		FRAME_TYPE_BEACON,
		FRAME_TYPE_PROBE_RESPONSE,
		FRAME_TYPE_ASSOC_RESPONSE
	} frametype = FRAME_TYPE_NONE;
	u8 *params = command + strlen(CMD_SETAPP2PWPSIE) + 1;
	int params_len = buf_len - strlen(CMD_SETAPP2PWPSIE) - 1;

	readbyte = slsi_str_to_int(&params[offset], (int *)&frametype);
	if (!readbyte) {
		SLSI_ERR(sdev, "frametype: failed to read a numeric value\n");
		result = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	readbyte = slsi_str_to_int(&params[offset], (int *)&iftype);
	if (!readbyte) {
		SLSI_ERR(sdev, "iftype: failed to read a numeric value\n");
		result = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	params_len = params_len - offset;
	SLSI_NET_DBG2(dev, SLSI_NETDEV, "command = %s, frametype=%d, iftype = %d, total buf_len=%d, params_len=%d\n", command, frametype, iftype, buf_len, params_len);

	/* check the net device interface type */
	if (iftype == IF_TYPE_P2P_DEVICE) {
		u8                *probe_resp_ie = NULL; /* params+offset; */

		SLSI_NET_DBG2(dev, SLSI_NETDEV,  "P2P device case");
		if (frametype != FRAME_TYPE_PROBE_RESPONSE) {
			SLSI_NET_ERR(dev, "Wrong frame type received\n");
			goto exit;
		}
		probe_resp_ie = kmalloc(params_len, GFP_KERNEL);
		if (probe_resp_ie == NULL) {
			SLSI_ERR(sdev, "Malloc for IEs failed\n");
			return -ENOMEM;
		}

		memcpy(probe_resp_ie, params+offset, params_len);

		SLSI_NET_DBG2(dev, SLSI_NETDEV, "P2P Device: probe_resp_ie is NOT NULL\n");
		return slsi_p2p_dev_probe_rsp_ie(sdev, dev, probe_resp_ie, params_len);
	} else if (iftype == IF_TYPE_P2P_INTERFACE) {
		SLSI_NET_DBG2(dev, SLSI_NETDEV,  "P2P GO case");
		if (frametype == FRAME_TYPE_BEACON) {
			SLSI_DBG1(sdev, SLSI_MLME, "P2P GO beacon IEs update\n");
			return slsi_p2p_go_vendor_ies_write(sdev, dev, params+offset, params_len, FAPI_PURPOSE_BEACON);
		} else if (frametype == FRAME_TYPE_PROBE_RESPONSE) {
			SLSI_DBG1(sdev, SLSI_MLME, "P2P GO proberesp IEs update\n");
			return slsi_p2p_go_vendor_ies_write(sdev, dev, params+offset, params_len, FAPI_PURPOSE_PROBE_RESPONSE);
		} else if (frametype == FRAME_TYPE_ASSOC_RESPONSE) {
			SLSI_DBG1(sdev, SLSI_MLME, "P2P GO Association Response IEs update\n");
			return slsi_p2p_go_vendor_ies_write(sdev, dev, params+offset, params_len, FAPI_PURPOSE_ASSOCIATION_RESPONSE);
		}
	}
exit:
	return result;
}

/**
 * P2P_LO_START handling.
 * Add unsync vif, register for action frames and set the listen channel.
 * The probe response IEs would be configured later.
 */
static int slsi_p2p_lo_start(struct net_device *dev, char *command)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	struct ieee80211_channel *chan = NULL;
	char  *lo_params = NULL;
	unsigned int channel, duration, interval, count;
	int  ret = 0;
	int  freq;
	int  readbyte = 0;
	enum ieee80211_band band;
	int  offset = 0;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	/* Reject LO if other operations are in progress. Back to back LO can be received.
	 * In such a case, if state is Listening then the listen offload flag should be true else
	 * reject the request as the Listening state would then be due to ROC.
	 */
	if ((sdev->p2p_state == P2P_SCANNING) || (sdev->p2p_state > P2P_LISTENING) ||
	    ((sdev->p2p_state == P2P_LISTENING) && (!ndev_vif->unsync.listen_offload))) {
		SLSI_NET_ERR(dev, "Reject LO due to ongoing P2P operation (state: %s)\n", slsi_p2p_state_text(sdev->p2p_state));
		ret = -EINVAL;
		goto exit;
	}

	SLSI_NET_DBG2(dev, SLSI_CFG80211, "Start Listen Offloading\n");

	lo_params = command + strlen(CMD_P2PLOSTART) + 1;
	readbyte = slsi_str_to_int(&lo_params[offset], &channel);
	if (!readbyte) {
		SLSI_ERR(sdev, "channel: failed to read a numeric value\n");
		ret = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	readbyte = slsi_str_to_int(&lo_params[offset], &duration);
	if (!readbyte) {
		SLSI_ERR(sdev, "duration: failed to read a numeric value\n");
		ret = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	readbyte = slsi_str_to_int(&lo_params[offset], &interval);
	if (!readbyte) {
		SLSI_ERR(sdev, "interval: failed to read a numeric value\n");
		ret = -EINVAL;
		goto exit;
	}
	offset = offset + readbyte + 1;
	readbyte = slsi_str_to_int(&lo_params[offset], &count);
	if (!readbyte) {
		SLSI_ERR(sdev, "count: failed to read a numeric value\n");
		ret = -EINVAL;
		goto exit;
	}

	if (!ndev_vif->activated) {
		ret = slsi_mlme_add_vif(sdev, dev, dev->dev_addr, dev->dev_addr);
		if (ret != 0) {
			SLSI_NET_ERR(dev, "Unsync vif addition failed\n");
			goto exit;
		}

		ndev_vif->activated = true;
		ndev_vif->mgmt_tx_data.exp_frame = SLSI_P2P_PA_INVALID;
		SLSI_P2P_STATE_CHANGE(sdev, P2P_IDLE_VIF_ACTIVE);

		ret = slsi_mlme_register_action_frame(sdev, dev, SLSI_ACTION_FRAME_PUBLIC, SLSI_ACTION_FRAME_PUBLIC);
		if (ret != 0) {
			SLSI_NET_ERR(dev, "Action frame registration for unsync vif failed\n");
			goto exit_with_vif_deactivate;
		}
	}

	/* Send set_channel irrespective of the values of LO parameters as they are not cached
	 * in driver to check whether they have changed.
	 */
	band = (channel <= 14) ? IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ;
	freq = ieee80211_channel_to_frequency(channel, band);
	chan = ieee80211_get_channel(sdev->wiphy, freq);
	if (!chan) {
		SLSI_NET_ERR(dev, "Incorrect channel: %u - Listen Offload failed\n", channel);
		ret = -EINVAL;
		goto exit_with_vif_deactivate;
	}

	ret = slsi_mlme_set_channel(sdev, dev, chan, duration, interval, count);
	if (ret != 0) {
		SLSI_NET_ERR(dev, "Set channel for unsync vif failed\n");
		goto exit_with_vif_deactivate;
	} else {
		ndev_vif->chan = chan;
	}

	ndev_vif->unsync.listen_offload = true;
	SLSI_P2P_STATE_CHANGE(ndev_vif->sdev, P2P_LISTENING);
	goto exit;

exit_with_vif_deactivate:
	slsi_p2p_vif_deactivate(sdev, dev, true);
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

/**
 * P2P_LO_STOP handling.
 * Queues a delayed work for unsync vif deletion similar to ROC duration expiry.
 * Clear listen offload flag and Probe response IEs.
 */
static int slsi_p2p_lo_stop(struct net_device *dev)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_DBG2(dev, SLSI_CFG80211, "Stop Listen Offloading\n");

	WARN_ON((!ndev_vif->unsync.listen_offload) || (ndev_vif->sdev->p2p_state != P2P_LISTENING));

	ndev_vif->unsync.listen_offload = false;

	if (slsi_mlme_add_info_elements(ndev_vif->sdev, dev, FAPI_PURPOSE_PROBE_RESPONSE, NULL, 0) != 0)
		SLSI_NET_ERR(dev, "Clearing Probe Response IEs for unsync vif failed\n");

	if (ndev_vif->sdev->p2p_state == P2P_LISTENING) {
		slsi_p2p_queue_unsync_vif_del_work(ndev_vif, SLSI_P2P_UNSYNC_VIF_EXTRA_MSEC);
		SLSI_P2P_STATE_CHANGE(ndev_vif->sdev, P2P_IDLE_VIF_ACTIVE);
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return 0;
}

static ssize_t slsi_rx_filter_num_write(struct net_device *dev, int add_remove, int filter_num)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               ret = 0;

	if (add_remove)
		sdev->device_config.rx_filter_num = filter_num;
	else
		sdev->device_config.rx_filter_num = 0;
	return ret;
}

static ssize_t slsi_set_country_rev(struct net_device *dev, char *country_code)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	char              alpha2_rev[4];
	int               status = 0;

	if (!country_code)
		return -EINVAL;

	memcpy(alpha2_rev, country_code, 4);

	status = slsi_set_country_update_regd(sdev, alpha2_rev, 4);

	return status;
}

static ssize_t slsi_get_country_rev(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	u8                buf[5];
	int               len = 0;

	memset(buf, 0, sizeof(buf));

	len = snprintf(command, buf_len, "%s %c%c %d", CMD_GETCOUNTRYREV,
		       sdev->device_config.domain_info.regdomain->alpha2[0],
		       sdev->device_config.domain_info.regdomain->alpha2[1],
		       sdev->device_config.domain_info.regdomain->dfs_region);

	return len;
}

#ifdef CONFIG_SCSC_WLAN_WES_NCHO
static ssize_t slsi_roam_scan_trigger_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_RSSI_ROAM_SCAN_TRIGGER, mib_value);
}

static ssize_t slsi_roam_scan_trigger_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_RSSI_ROAM_SCAN_TRIGGER, &mib_value);
	if (res)
		return res;
	res = snprintf(command, buf_len, "%s %d", CMD_GETROAMTRIGGER, mib_value);
	return res;
}

static ssize_t slsi_roam_delta_trigger_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_RSSI_ROAM_DELTA_TRIGGER, mib_value);
}

static ssize_t slsi_roam_delta_trigger_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_RSSI_ROAM_DELTA_TRIGGER, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETROAMDELTA, mib_value);
	return res;
}

static ssize_t slsi_cached_channel_scan_period_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_CACHED_CHANNEL_SCAN_PERIOD, mib_value * 1000000);
}

static ssize_t slsi_cached_channel_scan_period_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_CACHED_CHANNEL_SCAN_PERIOD, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETROAMSCANPERIOD, mib_value / 1000000);

	return res;
}

static ssize_t slsi_full_roam_scan_period_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_FULL_ROAM_SCAN_PERIOD, mib_value * 1000000);
}

static ssize_t slsi_full_roam_scan_period_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_FULL_ROAM_SCAN_PERIOD, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETFULLROAMSCANPERIOD, mib_value / 1000000);

	return res;
}

static ssize_t slsi_roam_scan_max_active_channel_time_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_MAX_ACTIVE_CHANNEL_TIME, mib_value);
}

static ssize_t slsi_roam_scan_max_active_channel_time_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_SCAN_MAX_ACTIVE_CHANNEL_TIME, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETSCANCHANNELTIME, mib_value);

	return res;
}

static ssize_t slsi_roam_scan_min_active_channel_time_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_MIN_ACTIVE_CHANNEL_TIME, mib_value);
}

static ssize_t slsi_roam_scan_min_active_channel_time_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_SCAN_MIN_ACTIVE_CHANNEL_TIME, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETROAMSCANCHANNELMINTIME, mib_value);

	return res;
}

static ssize_t slsi_roam_scan_probe_interval_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_NPROBE, mib_value);
}

static ssize_t slsi_roam_scan_probe_interval_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_SCAN_NPROBE, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETSCANNPROBES, mib_value);

	return res;
}

static ssize_t slsi_roam_mode_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	if (slsi_is_232338_test_mode_enabled()) {
		SLSI_DBG1_NODEV(SLSI_MLME, "SLSI_PSID_UNIFI_ROAM_MODE is not supported because of *#232338# rf test mode.\n");
		return -ENOTSUPP;
	}

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_MODE, mib_value);
}

static ssize_t slsi_roam_mode_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_MODE, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETROAMMODE, mib_value);

	return res;
}

static int slsi_roam_offload_ap_list(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif          *ndev_vif = netdev_priv(dev);
	struct slsi_dev            *sdev = ndev_vif->sdev;
	struct cfg80211_acl_data  *mac_acl;
	int                        ap_count = 0;
	int                        buf_pos = 0;
	int                        i, r;
	int                        malloc_len;

	/* command format:
	 *     x,aa:bb:cc:dd:ee:ff,xx:yy:zz:qq:ww:ee...
	 *     x = 1 to 100
	 *     each mac address id 17 bytes and every mac address is separated by ','
	 */
	buf_pos = slsi_str_to_int(command, &ap_count);
	SLSI_DBG1_NODEV(SLSI_MLME, "ap_count: %d\n", ap_count);
	if (ap_count < ROAMOFFLAPLIST_MIN || ap_count > ROAMOFFLAPLIST_MAX) {
		SLSI_ERR(sdev, "ap_count: %d\n", ap_count);
		return -EINVAL;
	}
	buf_pos++;
	/* each mac address takes 18 bytes(17 for mac address and 1 for ',') except the last one.
	 * the last mac address is just 17 bytes(without a coma)
	 */
	if ((buf_len - buf_pos) < (ap_count*18 - 1)) {
		SLSI_ERR(sdev, "Invalid buff len:%d for %d APs\n", (buf_len - buf_pos), ap_count);
		return -EINVAL;
	}
	malloc_len = sizeof(struct cfg80211_acl_data) + sizeof(struct mac_address) * ap_count;
	mac_acl = kmalloc(malloc_len, GFP_KERNEL);
	if (!mac_acl) {
		SLSI_ERR(sdev, "MEM fail for size:%ld\n", sizeof(struct cfg80211_acl_data) + sizeof(struct mac_address) * ap_count);
		return -ENOMEM;
	}

	for (i = 0; i < ap_count; i++) {
		slsi_machexstring_to_macarray(&command[buf_pos], mac_acl->mac_addrs[i].addr);
		buf_pos += 18;
		SLSI_DBG3_NODEV(SLSI_MLME, "[%pM]", mac_acl->mac_addrs[i].addr);
	}
	mac_acl->acl_policy = NL80211_ACL_POLICY_DENY_UNLESS_LISTED;
	mac_acl->n_acl_entries = ap_count;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	r = slsi_mlme_set_acl(sdev, dev, mac_acl);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	kfree(mac_acl);
	return r;
}

static ssize_t slsi_ccx_mode_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_CCX_SUPPORTED_VERSION, mib_value);
}

static ssize_t slsi_ccx_mode_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_CCX_SUPPORTED_VERSION, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETCCXMODE, (mib_value > 0) ? 1 : 0);

	return res;
}

static ssize_t slsi_roam_scan_band_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_BAND, mib_value);
}

static ssize_t slsi_roam_scan_band_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_SCAN_BAND, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETROAMINTRABAND, mib_value);

	return res;
}

static ssize_t slsi_freq_band_write(struct net_device *dev, uint band)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;

	slsi_band_update(sdev, band);
	/* Convert to correct Mib value (intra_band:1, all_band:2) */
	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_BAND, (band == SLSI_FREQ_BAND_AUTO) ? 2 : 1);
}

static ssize_t slsi_freq_band_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	char              buf[128];
	int               pos = 0;
	const size_t      bufsz = sizeof(buf);

	memset(buf, '\0', 128);
	pos += scnprintf(buf + pos, bufsz - pos, "Band %d", sdev->device_config.supported_band);

	buf[pos] = '\0';
	memcpy(command, buf, pos + 1);

	return pos;
}

static ssize_t slsi_dfs_scan_mode_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;
	char              buf[128];
	int               pos = 0;
	const size_t      bufsz = sizeof(buf);

	memset(buf, '\0', 128);
	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_DFS_SCAN_MODE, &mib_value);
	if (res)
		return res;

	pos += scnprintf(buf + pos, bufsz - pos, "%s %d", CMD_GETDFSSCANMODE, mib_value);

	buf[pos] = '\0';
	memcpy(command, buf, pos + 1);

	return pos;
}

static ssize_t slsi_roam_scan_control_write(struct net_device *dev, int mode)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);

	if (mode == 0 || mode == 1) {
		sdev->device_config.roam_scan_mode = mode;
	} else {
		SLSI_ERR(sdev, "Invalid roam Mode: Must be 0 or, 1 Not '%c'\n", mode);
		SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
		return -EINVAL;
	}

	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_CONTROL, sdev->device_config.roam_scan_mode);
}

static ssize_t slsi_roam_scan_control_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_SCAN_CONTROL, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETROAMSCANCONTROL, mib_value);

	return res;
}

static ssize_t slsi_roam_scan_home_time_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_HOME_TIME, mib_value);
}

static ssize_t slsi_roam_scan_home_time_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_SCAN_HOME_TIME, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETSCANHOMETIME, mib_value);

	return res;
}

static ssize_t slsi_roam_scan_home_away_time_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;

	slsi_str_to_int(command, &mib_value);
	SLSI_DBG1_NODEV(SLSI_MLME, "Setting to: %d", mib_value);

	return slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAM_SCAN_HOME_AWAY_TIME, mib_value);
}

static ssize_t slsi_roam_scan_home_away_time_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               mib_value = 0;
	int               res;

	res = slsi_get_mib_roam(sdev, SLSI_PSID_UNIFI_ROAM_SCAN_HOME_AWAY_TIME, &mib_value);
	if (res)
		return res;

	res = snprintf(command, buf_len, "%s %d", CMD_GETSCANHOMEAWAYTIME, mib_value);

	return res;
}

static ssize_t slsi_roam_scan_channels_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               result = 0;
	int               i, channel_count;
	int               offset = 0;
	int               readbyte = 0;
	int               channels[MAX_CHANNEL_LIST];

	readbyte = slsi_str_to_int(command, &channel_count);

	if (!readbyte) {
		SLSI_ERR(sdev, "channel count: failed to read a numeric value");
		return -EINVAL;
	}
	SLSI_MUTEX_LOCK(sdev->device_config_mutex);

	if (channel_count > MAX_CHANNEL_LIST)
		channel_count = MAX_CHANNEL_LIST;
	sdev->device_config.wes_roam_scan_list.n = channel_count;

	for (i = 0; i < channel_count; i++) {
		offset = offset + readbyte + 1;
		readbyte = slsi_str_to_int(&command[offset], &channels[i]);
		if (!readbyte) {
			SLSI_ERR(sdev, "failed to read a numeric value\n");
			SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
			return -EINVAL;
		}

		sdev->device_config.wes_roam_scan_list.channels[i] = channels[i];
	}
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	result = slsi_mlme_set_cached_channels(sdev, dev, channel_count, sdev->device_config.wes_roam_scan_list.channels);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return result;
}

static ssize_t slsi_roam_scan_channels_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	char              channel_buf[128] = { 0 };
	int               pos = 0;
	int               i;
	int               channel_count = 0;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	channel_count = sdev->device_config.wes_roam_scan_list.n;
	pos = scnprintf(channel_buf, sizeof(channel_buf), "%s %d", CMD_GETROAMSCANCHANNELS, channel_count);
	for (i = 0; i < channel_count; i++)
		pos += scnprintf(channel_buf + pos, sizeof(channel_buf) - pos, " %d", sdev->device_config.wes_roam_scan_list.channels[i]);
	channel_buf[pos] = '\0';

	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);

	memcpy(command, channel_buf, pos + 1);

	return pos;
}

static ssize_t slsi_okc_mode_write(struct net_device *dev, int mode)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);

	if (mode == 0 || mode == 1) {
		sdev->device_config.okc_mode = mode;
	} else {
		SLSI_ERR(sdev, "Invalid OKC Mode: Must be 0 or, 1 Not '%c'\n", mode);
		SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
		return -EINVAL;
	}

	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	return 0;
}

static ssize_t slsi_okc_mode_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               okc_mode;
	int               res;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	okc_mode = sdev->device_config.okc_mode;
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);

	res = snprintf(command, buf_len, "%s %d", CMD_GETOKCMODE, okc_mode);

	return res;
}

static ssize_t slsi_wes_mode_write(struct net_device *dev, int mode)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               result = 0;
	u32               action_frame_bmap = SLSI_STA_ACTION_FRAME_BITMAP;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);

	if (mode == 0 || mode == 1) {
		sdev->device_config.wes_mode = mode;
	} else {
		SLSI_ERR(sdev, "Invalid WES Mode: Must be 0 or 1 Not '%c'\n", mode);
		SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
		return -EINVAL;
	}
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if ((ndev_vif->activated) && (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) &&
	    (ndev_vif->sta.vif_status == SLSI_VIF_STATUS_CONNECTED)) {
		if (sdev->device_config.wes_mode)
			action_frame_bmap |= SLSI_ACTION_FRAME_VENDOR_SPEC;

		result = slsi_mlme_register_action_frame(sdev, dev, action_frame_bmap, action_frame_bmap);
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return result;
}

static ssize_t slsi_wes_mode_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int               wes_mode;
	int               res;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	wes_mode = sdev->device_config.wes_mode;
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);

	res = snprintf(command, buf_len, "%s %d", CMD_GETWESMODE, wes_mode);

	return res;
}
#endif

static ssize_t slsi_set_pmk(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	u8                pmk[33];
	int               result = 0;

	memcpy((u8 *)pmk, command + strlen("SET_PMK "), 32);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	result = slsi_mlme_set_pmk(sdev, dev, pmk, 32);

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return result;
}

static ssize_t slsi_auto_chan_read(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = ndev_vif->sdev;
	int             ap_auto_chan;
	int result = 0;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	ap_auto_chan = sdev->device_config.ap_auto_chan;
	SLSI_INFO(sdev, "Auto channel selected: %d\n", ap_auto_chan);
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);

	result = snprintf(command, buf_len, "%s %d", CMD_HAPD_GET_CHANNEL, ap_auto_chan);
	return result;
}

static ssize_t slsi_auto_chan_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif        *ndev_vif = netdev_priv(dev);
	struct slsi_dev          *sdev = ndev_vif->sdev;
	int                      n_channels;
	struct ieee80211_channel *channels[SLSI_NO_OF_SCAN_CHANLS_FOR_AUTO_CHAN_MAX] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	int                      count_channels;
	int                      offset;
	int                      chan;

	offset = slsi_str_to_int(&command[21], &n_channels);
	if (!offset) {
		SLSI_ERR(sdev, "channel count: failed to read a numeric value");
		return -EINVAL;
	}

	SLSI_DBG3(sdev, SLSI_CFG80211, "n_channels:%d\n", n_channels);
	if (n_channels > SLSI_NO_OF_SCAN_CHANLS_FOR_AUTO_CHAN_MAX) {
		SLSI_ERR(sdev, "channel count:%d > SLSI_NO_OF_SCAN_CHANLS_FOR_AUTO_CHAN_MAX:%d\n", n_channels, SLSI_NO_OF_SCAN_CHANLS_FOR_AUTO_CHAN_MAX);
		return -EINVAL;
	}

	/* If "1 6 11" are passed, scan all "1 - 14" channels. If "1 6" are passed, scan "1 - 9" channels */
	if (n_channels == 3)
		n_channels = 14;
	else if (n_channels == 2)
		n_channels = 9;
	SLSI_DBG3(sdev, SLSI_INIT_DEINIT, "Number of channels to scan = %d", n_channels);

	count_channels = 0;
	for (chan = 1; chan <= n_channels; chan++) {
		int center_freq;

		center_freq = ieee80211_channel_to_frequency(chan, NL80211_BAND_2GHZ);
		channels[count_channels] = ieee80211_get_channel(sdev->wiphy, center_freq);
		if (!channels[count_channels])
			SLSI_WARN(sdev, "channel number:%d invalid\n", chan);
		else
			count_channels++;

		SLSI_DBG3(sdev, SLSI_INIT_DEINIT, "channels[%d]:%d\n", count_channels, chan);
	}

	SLSI_DBG3(sdev, SLSI_INIT_DEINIT, "Number of channels for autchannel selection= %d", count_channels);

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	sdev->device_config.ap_auto_chan = 0;
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);

	return slsi_auto_chan_select_scan(sdev, count_channels, channels);
}

static ssize_t slsi_reassoc_write(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	struct slsi_dev     *sdev = ndev_vif->sdev;
	u8                  bssid[6] = { 0 };
	int                 channel;
	int                 freq;
	enum ieee80211_band band = IEEE80211_BAND_2GHZ;
	int                 r = 0;

	if (command[17] != ' ') {
		SLSI_ERR(sdev, "Invalid Format '%s' '%c'\n", command, command[17]);
		return -EINVAL;
	}

	command[17] = '\0';

	slsi_machexstring_to_macarray(command, bssid);
	SLSI_DBG1_NODEV(SLSI_MLME, "ROAM BSSID '%pM'\n", bssid);

	if (!slsi_str_to_int(&command[18], &channel)) {
		SLSI_ERR(sdev, "Invalid channel string: '%s'\n", &command[18]);
		return -EINVAL;
	}

	SLSI_DBG1_NODEV(SLSI_MLME, "ROAM to channel '%d'\n", channel);

	if (channel > 14)
		band = IEEE80211_BAND_5GHZ;
	freq = (u16)ieee80211_channel_to_frequency(channel, band);

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	r = slsi_mlme_roam(sdev, dev, bssid, freq);

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

static ssize_t slsi_send_action_frame(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif    *ndev_vif = netdev_priv(dev);
	struct slsi_dev      *sdev = ndev_vif->sdev;
	char                 *temp;
	u8                   bssid[6] = { 0 };
	int                  channel = 0;
	int                  freq = 0;
	enum ieee80211_band  band = IEEE80211_BAND_2GHZ;
	int                  r = 0;
	u16                  host_tag = slsi_tx_host_tag(sdev);
	u32                  dwell_time;
	struct ieee80211_hdr *hdr;
	u8                   *buf = NULL;
	u8                   *final_buf = NULL;
	u8                   temp_byte;
	int                  len = 0;
	int                  final_length = 0;
	int                  i = 0, j = 0;
	char                 *pos;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if ((!ndev_vif->activated) || (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) ||
	    (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED)) {
		SLSI_ERR(sdev, "Not a STA vif or status is not CONNECTED\n");
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -EINVAL;
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	command[17] = '\0';
	slsi_machexstring_to_macarray(command, bssid);

	command[17] = ' ';
	SLSI_DBG1_NODEV(SLSI_MLME, "BSSID '%pM'\n", bssid);

	pos = strchr(command, ' ');
	if (pos == NULL)
		return -EINVAL;
	*pos++ = '\0';

	if (!slsi_str_to_int(pos, &channel)) {
		SLSI_ERR(sdev, "Invalid channel string: '%s'\n", pos);
		return -EINVAL;
	}
	pos++;

	if (channel > 14)
		band = IEEE80211_BAND_5GHZ;
	freq = (u16)ieee80211_channel_to_frequency(channel, band);

	SLSI_DBG1_NODEV(SLSI_MLME, "freq %d\n", freq);

	pos = strchr(pos, ' ');
	if (pos == NULL)
		return -EINVAL;
	*pos++ = '\0';

	if (!slsi_str_to_int(pos, &dwell_time)) {
		SLSI_ERR(sdev, "Invalid dwell time string: '%s'\n", pos);
		return -EINVAL;
	}

	pos = strchr(pos, ' ');
	if (pos == NULL)
		return -EINVAL;
	pos++;

	/*Length of data*/
	temp = pos;
	while (*temp != '\0')
		temp++;
	len = temp - pos;

	if (len <= 0)
		return -EINVAL;
	buf = kmalloc((len + 1) / 2, GFP_KERNEL);

	if (buf == NULL) {
		SLSI_ERR(sdev, "Malloc  failed\n");
		return -ENOMEM;
	}
	/*We receive a char buffer, convert to hex*/
	temp = pos;
	for (i = 0, j = 0; j < len; j += 2) {
		if (j + 1 == len)
			temp_byte = slsi_parse_hex(temp[j]);
		else
			temp_byte = slsi_parse_hex(temp[j]) << 4 | slsi_parse_hex(temp[j + 1]);
		buf[i++] = temp_byte;
	}
	len = i;

	final_length = len + IEEE80211_HEADER_SIZE;
	final_buf = kmalloc(final_length, GFP_KERNEL);
	if (final_buf == NULL) {
		SLSI_ERR(sdev, "Malloc  failed\n");
		kfree(buf);
		return -ENOMEM;
	}

	hdr = (struct ieee80211_hdr *)final_buf;
	hdr->frame_control = IEEE80211_FC(IEEE80211_FTYPE_MGMT, IEEE80211_STYPE_ACTION);
	SLSI_ETHER_COPY(hdr->addr1, bssid);
	SLSI_ETHER_COPY(hdr->addr2, sdev->hw_addr);
	SLSI_ETHER_COPY(hdr->addr3, bssid);
	memcpy(final_buf + IEEE80211_HEADER_SIZE, buf, len);

	kfree(buf);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	r = slsi_mlme_send_frame_mgmt(sdev, dev, final_buf, final_length, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, SLSI_FREQ_HOST_TO_FW(freq), dwell_time * 1000, 0);

	kfree(final_buf);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

static ssize_t slsi_setting_max_sta_write(struct net_device *dev, int sta_number)
{
	struct netdev_vif    *ndev_vif = netdev_priv(dev);
	struct slsi_dev      *sdev = ndev_vif->sdev;
	struct slsi_mib_data mib_data = { 0, NULL };
	int                  result = 0;

	SLSI_INFO(sdev, "Setting max_num_sta to %d\n", sta_number);

	if (sta_number > 10 || sta_number < 1)
		return -EINVAL;
	result = slsi_mib_encode_uint(&mib_data, SLSI_PSID_UNIFI_MAX_CLIENT, sta_number, 0);
	if ((result != SLSI_MIB_STATUS_SUCCESS) || (mib_data.dataLength == 0))
		return -ENOMEM;
	result = slsi_mlme_set(sdev, dev, mib_data.data, mib_data.dataLength);
	if (result != 0)
		SLSI_ERR(sdev, "max_sta: mlme_set_req failed: Result code: %d\n", result);
	kfree(mib_data.data);

	return result;
}

static ssize_t slsi_country_write(struct net_device *dev, char *country_code)
{
	struct netdev_vif *netdev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = netdev_vif->sdev;
	char              alpha2_code[SLSI_COUNTRY_CODE_LEN];
	int               status;

	if (strlen(country_code) < 2)
		return -EINVAL;

	memcpy(alpha2_code, country_code, 2);
	alpha2_code[2] = ' '; /* set 3rd byte of countrycode to ASCII space */

	status = slsi_set_country_update_regd(sdev, alpha2_code, SLSI_COUNTRY_CODE_LEN);

	return status;
}

static ssize_t slsi_gk_key_write(struct net_device *dev, char *command, int buf_len)
{
#define CCX_KRK_LEN 16
#define CCX_BTK_LEN 32
	struct netdev_vif        *netdev_vif = netdev_priv(dev);
	struct slsi_dev          *sdev = netdev_vif->sdev;
	int               r = -EINVAL;
	s32              vif;
	u8                gk_key[CCX_KRK_LEN+CCX_BTK_LEN];

	memcpy((u8 *)gk_key, command, (CCX_BTK_LEN+CCX_KRK_LEN));

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	for (vif = 1; vif <= CONFIG_SCSC_WLAN_MAX_INTERFACES; vif++) {
		SLSI_MUTEX_LOCK(netdev_vif->vif_mutex);

		if (!netdev_vif->activated) {
			SLSI_MUTEX_UNLOCK(netdev_vif->vif_mutex);
			continue;
		}

		if ((netdev_vif->vif_type == FAPI_VIFTYPE_STATION) &&
		    (netdev_vif->iftype == NL80211_IFTYPE_STATION)) {
			r = slsi_mlme_set_cckm_key(sdev, dev, gk_key);
			SLSI_MUTEX_UNLOCK(netdev_vif->vif_mutex);
			break;
		}
		SLSI_MUTEX_UNLOCK(netdev_vif->vif_mutex);
	}
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);

	return r;
}

static int slsi_tdls_channel_switch(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif  *ndev_vif = netdev_priv(dev);
	struct slsi_dev    *sdev = ndev_vif->sdev;
	int                r, len_processed;
	u8                 peer_mac[6];
	u32                center_freq = 0;
	u32                chan_info = 0;
	int                is_ch_switch;
	struct slsi_peer   *peer;

/*  Command format:
 *      [0/1] [mac address] [center frequency] [channel_info]
 *  channel switch: "1 00:01:02:03:04:05 2412 20"
 *  cancel channel switch: "0 00:01:02:03:04:05"
 */
/* switch/cancel(1 byte) space(1byte) macaddress 17 char */
#define SLSI_TDLS_IOCTL_CMD_DATA_MIN_LEN 19

	if (buf_len < SLSI_TDLS_IOCTL_CMD_DATA_MIN_LEN) {
		SLSI_NET_ERR(dev, "buf len should be atleast %d. but is:%d\n", SLSI_TDLS_IOCTL_CMD_DATA_MIN_LEN, buf_len);
		return -EINVAL;
	}

	if (ndev_vif->sta.sta_bss == NULL) {
		SLSI_NET_ERR(dev, "sta_bss is not available\n");
		return -EINVAL;
	}

	is_ch_switch = command[0] - '0';
	buf_len -= 2;
	command += 2;

	slsi_machexstring_to_macarray(command, peer_mac);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (is_ch_switch) {
		/* mac address(17 char) + space(1 char) = 18 */
		command += 18;
		buf_len -= 18;

		len_processed = slsi_str_to_int(command, &center_freq);
		/* +1 for space */
		buf_len -= len_processed + 1;
		command += len_processed + 1;
		if (buf_len <= 0) {
			SLSI_NET_ERR(dev, "No buf for chan_info\n");
			r = -EINVAL;
			goto exit;
		}
		buf_len -= slsi_str_to_int(command, &chan_info);

		if (((chan_info & 0xFF) != 20)  && ((chan_info & 0xFF) != 40)) {
			SLSI_NET_ERR(dev, "Invalid chan_info(%d)\n", chan_info);
			r = -EINVAL;
			goto exit;
		}
		/* In 2.4 Ghz channel 1(2412MHz) to channel 14(2484MHz) */
		/* for 40MHz channels are from 1 to 13, its 2422MHz to 2462MHz. */
		if ((((chan_info & 0xFF) == 20) && (center_freq < 2412 || center_freq > 2484)) ||
		    (((chan_info & 0xFF) == 40) && (center_freq < 2422 || center_freq > 2462))) {
			SLSI_NET_ERR(dev, "Invalid center_freq(%d) for chan_info(%d)\n", center_freq, chan_info);
			r = -EINVAL;
			goto exit;
		}
	} else {
		/* Incase of cancel channel switch fallback to bss channel */
		center_freq = ndev_vif->sta.sta_bss->channel->center_freq;
		chan_info = 20; /* Hardcoded to 20MHz as cert tests use BSS with 20MHz */
	}

	SLSI_NET_DBG2(dev, SLSI_TDLS, "ch_switch(%d) mac[%pM] freq(%d) chan_info(0x%02x)\n", is_ch_switch, peer_mac, center_freq, chan_info);

	peer = slsi_get_peer_from_mac(sdev, dev, peer_mac);

	if (!peer || !slsi_is_tdls_peer(dev, peer)) {
		SLSI_NET_ERR(dev, "%s peer aid:%d\n", peer ? "Invalid" : "No", peer ? peer->aid : 0);
		r = -EINVAL;
		goto exit;
	}

	r = slsi_mlme_tdls_action(sdev, dev, peer_mac, FAPI_TDLSACTION_CHANNEL_SWITCH, center_freq, chan_info);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

int slsi_set_tx_power_calling(struct net_device *dev, char *command, int buf_len)
{
	struct netdev_vif    *ndev_vif = netdev_priv(dev);
	struct slsi_dev      *sdev = ndev_vif->sdev;
	int                  mode;
	struct slsi_mib_data mib_data = { 0, NULL };
	int                  error = 0;

	(void)slsi_str_to_int(command, &mode);
	SLSI_DBG1(sdev, SLSI_MLME, "Mode:%d\n", mode);

	error = slsi_mib_encode_uint(&mib_data, SLSI_PSID_UNIFI_SAR_BACKOFF_ENABLE, mode == 0 ? 1 : 0, 0);
	if ((error != SLSI_MIB_STATUS_SUCCESS) || (mib_data.dataLength == 0))
		return -ENOMEM;

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	error = slsi_mlme_set(sdev, NULL, mib_data.data, mib_data.dataLength);
	if (error != 0)
		SLSI_ERR(sdev, "mlme_set_req failed:%d\n", error);
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	kfree(mib_data.data);

	return error;
}

static int slsi_print_regulatory(struct slsi_802_11d_reg_domain *domain_info, char *buf, int buf_len)
{
	int  cur_pos = 0;
	int  i;
	char *dfs_region_str[] = {"unknown", "ETSI", "FCC", "JAPAN", "GLOBAL", "CHINA"};
	u8   dfs_region_index;
	struct ieee80211_reg_rule *reg_rule;

	cur_pos = snprintf(buf, buf_len, "country %c%c:", domain_info->regdomain->alpha2[0],
			   domain_info->regdomain->alpha2[1]);
	dfs_region_index = domain_info->regdomain->dfs_region <= 5 ? domain_info->regdomain->dfs_region : 0;
	cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, "DFS-%s\n", dfs_region_str[dfs_region_index]);
	for (i = 0; i < domain_info->regdomain->n_reg_rules; i++) {
		reg_rule = &domain_info->regdomain->reg_rules[i];
		cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, "\t(%d-%d @ %d), (N/A, %d)",
					reg_rule->freq_range.start_freq_khz/1000,
					reg_rule->freq_range.end_freq_khz/1000,
					reg_rule->freq_range.max_bandwidth_khz/1000,
					MBM_TO_DBM(reg_rule->power_rule.max_eirp));
		if (reg_rule->flags) {
			if (reg_rule->flags & NL80211_RRF_DFS)
				cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", DFS");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 9))
			if (reg_rule->flags & NL80211_RRF_NO_OFDM)
				cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", NO_OFDM");
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0))
			if (reg_rule->flags & (NL80211_RRF_PASSIVE_SCAN|NL80211_RRF_NO_IBSS))
				cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", NO_IR");
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
			if (reg_rule->flags & (NL80211_RRF_NO_IR))
				cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", NO_IR");
#endif
			if (reg_rule->flags & NL80211_RRF_NO_INDOOR)
				cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", NO_INDOOR");
			if (reg_rule->flags & NL80211_RRF_NO_OUTDOOR)
				cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", NO_OUTDOOR");
		}
		cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, "\n");
	}
	/* All countries/regulatory domains allow channels 1-11. */
	cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, "Channels: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11");
	for (i = 0; i < domain_info->regdomain->n_reg_rules; i++) {
		reg_rule = &domain_info->regdomain->reg_rules[i];
		if ((reg_rule->freq_range.start_freq_khz <= 2467000 - 10000) &&
		    (reg_rule->freq_range.end_freq_khz >= 2467000 + 10000))
			cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", 12");
		if ((reg_rule->freq_range.start_freq_khz <= 2472000 - 10000) &&
		    (reg_rule->freq_range.end_freq_khz >= 2472000 + 10000))
			cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", 13");
		if ((reg_rule->freq_range.start_freq_khz <= 2484000 - 10000) &&
		    (reg_rule->freq_range.end_freq_khz >= 2484000 + 10000))
			cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, ", 14");
	}
	cur_pos += snprintf(buf + cur_pos, buf_len - cur_pos, "\n");
	return cur_pos;
}

static int slsi_get_regulatory(struct net_device *dev, char *buf, int buf_len)
{
	struct netdev_vif              *ndev_vif = netdev_priv(dev);
	struct slsi_dev                *sdev = ndev_vif->sdev;
	int                            mode;
	int                            cur_pos = 0;
	int                            status;
	u8                             alpha2[3];

	mode = buf[strlen(CMD_GETREGULATORY) + 1] - '0';
	if (mode == 1) {
		struct slsi_802_11d_reg_domain domain_info;

		memset(&domain_info, 0, sizeof(struct slsi_802_11d_reg_domain));
		SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
		if (!ndev_vif->activated || ndev_vif->vif_type != FAPI_VIFTYPE_STATION || !ndev_vif->sta.sta_bss) {
			cur_pos += snprintf(buf, buf_len - cur_pos, "Station not connected");
			SLSI_ERR(sdev, "station not connected. vif.activated:%d, vif.type:%d, vif.bss:%s\n",
				 ndev_vif->activated, ndev_vif->vif_type, ndev_vif->sta.sta_bss ? "yes" : "no");
			SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
			return -EINVAL;
		}
		/* read vif specific country code, index = vifid+1 */
		status = slsi_read_default_country(sdev, alpha2, ndev_vif->ifnum + 1);
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		if (status)
			return status;

		/* max 20 rules */
		domain_info.regdomain = kmalloc(sizeof(*domain_info.regdomain) + sizeof(struct ieee80211_reg_rule) * 20, GFP_KERNEL);
		if (!domain_info.regdomain) {
			SLSI_ERR(sdev, "no memory size:%lu\n",
				 sizeof(struct ieee80211_regdomain) + sizeof(struct ieee80211_reg_rule) * 20);
			return -ENOMEM;
		}

		/* get regulatory rules based on country code */
		domain_info.countrylist = sdev->device_config.domain_info.countrylist;
		domain_info.country_len = sdev->device_config.domain_info.country_len;
		status = slsi_read_regulatory_rules(sdev, &domain_info, alpha2);
		if (status) {
			kfree(domain_info.regdomain);
			return status;
		}

		cur_pos += slsi_print_regulatory(&domain_info, buf + cur_pos, buf_len - cur_pos);
		kfree(domain_info.regdomain);
	} else if (mode == 0) {
		SLSI_MUTEX_LOCK(sdev->device_config_mutex);
		cur_pos += slsi_print_regulatory(&sdev->device_config.domain_info, buf + cur_pos, buf_len - cur_pos);
		SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	} else {
		cur_pos += snprintf(buf, buf_len - cur_pos, "invalid option %d", mode);
		SLSI_ERR(sdev, "invalid option:%d\n", mode);
		return -EINVAL;
	}
	/* Buf is somewhere close to 4Kbytes. so expect some spare space. If there is no spare
	 * space we might have missed printing some text in buf.
	 */
	if (buf_len - cur_pos)
		return cur_pos;
	else
		return -ENOMEM;
}

int slsi_set_fcc_channel(struct net_device *dev, char *cmd, int cmd_len)
{
	struct netdev_vif    *ndev_vif = netdev_priv(dev);
	struct slsi_dev      *sdev = ndev_vif->sdev;
	struct slsi_mib_data mib_data = { 0, NULL };
	int                  status;
	bool                 max_power_ena;

	max_power_ena = (cmd[0]  == '0');
	SLSI_DBG3(sdev, SLSI_NETDEV, "mib value :%d\n", max_power_ena);

	status = slsi_mib_encode_bool(&mib_data, SLSI_PSID_UNIFI_MCD_MAX_POWER_ENA, max_power_ena, 0);
	if (status != SLSI_MIB_STATUS_SUCCESS) {
		SLSI_ERR(sdev, "SET_FCC_CHANNEL FAIL: no mem for MIB\n");
		return -ENOMEM;
	}

	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	status = slsi_mlme_set(sdev, NULL, mib_data.data, mib_data.dataLength);
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);

	kfree(mib_data.data);

	if (status)
		SLSI_ERR(sdev, "Err setting McdMaxPowerEna. error = %d\n", status);

	return status;
}

int slsi_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
#define MAX_LEN_PRIV_COMMAND    4096 /*This value is the max reply size set in supplicant*/
	struct android_wifi_priv_cmd priv_cmd;
	int                          ret = 0;
	u8                        *command = NULL;

	if (!dev) {
		ret = -ENODEV;
		goto exit;
	}

	if (!rq->ifr_data) {
		ret = -EINVAL;
		goto exit;
	}
	if (copy_from_user((void *)&priv_cmd, (void *)rq->ifr_data, sizeof(struct android_wifi_priv_cmd))) {
		ret = -EFAULT;
		SLSI_NET_ERR(dev, "ifr data failed\n");
		goto exit;
	}

	SLSI_NET_DBG3(dev, SLSI_CFG80211, "total_len in priv_cmd = %d\n", priv_cmd.total_len);
	if ((priv_cmd.total_len > MAX_LEN_PRIV_COMMAND) || (priv_cmd.total_len < 0)) {
		ret = -EINVAL;
		SLSI_NET_ERR(dev, "Length mismatch total_len = %d\n", priv_cmd.total_len);
		goto exit;
	}
	command = kmalloc((priv_cmd.total_len + 1), GFP_KERNEL);
	if (!command) {
		ret = -ENOMEM;
		SLSI_NET_ERR(dev, "No memory\n");
		goto exit;
	}
	if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		SLSI_NET_ERR(dev, "Buffer copy fail\n");
		goto exit;
	}
	command[priv_cmd.total_len] = '\0';

	if (strnicmp(command, CMD_SETSUSPENDMODE, strlen(CMD_SETSUSPENDMODE)) == 0) {
		ret = slsi_set_suspend_mode(dev, command);
	} else if (strnicmp(command, CMD_RXFILTERADD, strlen(CMD_RXFILTERADD)) == 0) {
		int filter_num = *(command + strlen(CMD_RXFILTERADD) + 1) - '0';

		ret = slsi_rx_filter_num_write(dev, 1, filter_num);
	} else if (strnicmp(command, CMD_RXFILTERREMOVE, strlen(CMD_RXFILTERREMOVE)) == 0) {
		int filter_num = *(command + strlen(CMD_RXFILTERREMOVE) + 1) - '0';

		ret = slsi_rx_filter_num_write(dev, 0, filter_num);
	} else if (strnicmp(command, CMD_SETCOUNTRYREV, strlen(CMD_SETCOUNTRYREV)) == 0) {
		char *country_code = command + strlen(CMD_SETCOUNTRYREV) + 1;

		ret = slsi_set_country_rev(dev, country_code);
	} else if (strnicmp(command, CMD_GETCOUNTRYREV, strlen(CMD_GETCOUNTRYREV)) == 0) {
		ret = slsi_get_country_rev(dev, priv_cmd.buf, priv_cmd.total_len);
#ifdef CONFIG_SCSC_WLAN_WES_NCHO
	} else if (strnicmp(command, CMD_SETROAMTRIGGER, strlen(CMD_SETROAMTRIGGER)) == 0) {
		int skip = strlen(CMD_SETROAMTRIGGER) + 1;

		ret = slsi_roam_scan_trigger_write(dev, command + skip,
						   priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETROAMTRIGGER, strlen(CMD_GETROAMTRIGGER)) == 0) {
		ret = slsi_roam_scan_trigger_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMDELTA, strlen(CMD_SETROAMDELTA)) == 0) {
		int skip = strlen(CMD_SETROAMDELTA) + 1;

		ret = slsi_roam_delta_trigger_write(dev, command + skip,
						    priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETROAMDELTA, strlen(CMD_GETROAMDELTA)) == 0) {
		ret = slsi_roam_delta_trigger_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMSCANPERIOD, strlen(CMD_SETROAMSCANPERIOD)) == 0) {
		int skip = strlen(CMD_SETROAMSCANPERIOD) + 1;

		ret = slsi_cached_channel_scan_period_write(dev, command + skip,
							    priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETROAMSCANPERIOD, strlen(CMD_GETROAMSCANPERIOD)) == 0) {
		ret = slsi_cached_channel_scan_period_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETFULLROAMSCANPERIOD, strlen(CMD_SETFULLROAMSCANPERIOD)) == 0) {
		int skip = strlen(CMD_SETFULLROAMSCANPERIOD) + 1;

		ret = slsi_full_roam_scan_period_write(dev, command + skip,
						       priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETFULLROAMSCANPERIOD, strlen(CMD_GETFULLROAMSCANPERIOD)) == 0) {
		ret = slsi_full_roam_scan_period_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETSCANCHANNELTIME, strlen(CMD_SETSCANCHANNELTIME)) == 0) {
		int skip = strlen(CMD_SETSCANCHANNELTIME) + 1;

		ret = slsi_roam_scan_max_active_channel_time_write(dev, command + skip,
								   priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETSCANCHANNELTIME, strlen(CMD_GETSCANCHANNELTIME)) == 0) {
		ret = slsi_roam_scan_max_active_channel_time_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMSCANCHANNELMINTIME, strlen(CMD_SETROAMSCANCHANNELMINTIME)) == 0) {
		int skip = strlen(CMD_SETROAMSCANCHANNELMINTIME) + 1;

		ret = slsi_roam_scan_min_active_channel_time_write(dev, command + skip,
								   priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETROAMSCANCHANNELMINTIME, strlen(CMD_GETROAMSCANCHANNELMINTIME)) == 0) {
		ret = slsi_roam_scan_min_active_channel_time_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETSCANNPROBES, strlen(CMD_SETSCANNPROBES)) == 0) {
		int skip = strlen(CMD_SETSCANNPROBES) + 1;

		ret = slsi_roam_scan_probe_interval_write(dev, command + skip,
							  priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETSCANNPROBES, strlen(CMD_GETSCANNPROBES)) == 0) {
		ret = slsi_roam_scan_probe_interval_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMMODE, strlen(CMD_SETROAMMODE)) == 0) {
		int skip = strlen(CMD_SETROAMMODE) + 1;

		ret = slsi_roam_mode_write(dev, command + skip,
					   priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETROAMMODE, strlen(CMD_GETROAMMODE)) == 0) {
		ret = slsi_roam_mode_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETCCXMODE, strlen(CMD_SETCCXMODE)) == 0) {
		int skip = strlen(CMD_SETCCXMODE) + 1;

		ret = slsi_ccx_mode_write(dev, command + skip,
					  priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETCCXMODE, strlen(CMD_GETCCXMODE)) == 0) {
		ret = slsi_ccx_mode_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMINTRABAND, strlen(CMD_SETROAMINTRABAND)) == 0) {
		int skip = strlen(CMD_SETROAMINTRABAND) + 1;

		ret = slsi_roam_scan_band_write(dev, command + skip,
						priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETROAMINTRABAND, strlen(CMD_GETROAMINTRABAND)) == 0) {
		ret = slsi_roam_scan_band_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMBAND, strlen(CMD_SETROAMBAND)) == 0) {
		uint band = *(command + strlen(CMD_SETROAMBAND) + 1) - '0';

		ret = slsi_freq_band_write(dev, band);
	} else if (strnicmp(command, CMD_SETBAND, strlen(CMD_SETBAND)) == 0) {
		uint band = *(command + strlen(CMD_SETBAND) + 1) - '0';

		ret = slsi_freq_band_write(dev, band);
	} else if ((strnicmp(command, CMD_GETROAMBAND, strlen(CMD_GETROAMBAND)) == 0) || (strnicmp(command, CMD_GETBAND, strlen(CMD_GETBAND)) == 0)) {
		ret = slsi_freq_band_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if ((strnicmp(command, CMD_GETDFSSCANMODE, strlen(CMD_GETDFSSCANMODE)) == 0)) {
		ret = slsi_dfs_scan_mode_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMSCANCONTROL, strlen(CMD_SETROAMSCANCONTROL)) == 0) {
		int mode = *(command + strlen(CMD_SETROAMSCANCONTROL) + 1) - '0';

		ret = slsi_roam_scan_control_write(dev, mode);
	} else if (strnicmp(command, CMD_GETROAMSCANCONTROL, strlen(CMD_GETROAMSCANCONTROL)) == 0) {
		ret = slsi_roam_scan_control_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETSCANHOMETIME, strlen(CMD_SETSCANHOMETIME)) == 0) {
		int skip = strlen(CMD_SETSCANHOMETIME) + 1;

		ret = slsi_roam_scan_home_time_write(dev, command + skip,
						     priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETSCANHOMETIME, strlen(CMD_GETSCANHOMETIME)) == 0) {
		ret = slsi_roam_scan_home_time_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETSCANHOMEAWAYTIME, strlen(CMD_SETSCANHOMEAWAYTIME)) == 0) {
		int skip = strlen(CMD_SETSCANHOMEAWAYTIME) + 1;

		ret = slsi_roam_scan_home_away_time_write(dev, command + skip,
							  priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETSCANHOMEAWAYTIME, strlen(CMD_GETSCANHOMEAWAYTIME)) == 0) {
		ret = slsi_roam_scan_home_away_time_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETOKCMODE, strlen(CMD_SETOKCMODE)) == 0) {
		int mode = *(command + strlen(CMD_SETOKCMODE) + 1) - '0';

		ret = slsi_okc_mode_write(dev, mode);
	} else if (strnicmp(command, CMD_GETOKCMODE, strlen(CMD_GETOKCMODE)) == 0) {
		ret = slsi_okc_mode_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETWESMODE, strlen(CMD_SETWESMODE)) == 0) {
		int mode = *(command + strlen(CMD_SETWESMODE) + 1) - '0';

		ret = slsi_wes_mode_write(dev, mode);
	} else if (strnicmp(command, CMD_GETWESMODE, strlen(CMD_GETWESMODE)) == 0) {
		ret = slsi_wes_mode_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMSCANCHANNELS, strlen(CMD_SETROAMSCANCHANNELS)) == 0) {
		int skip = strlen(CMD_SETROAMSCANCHANNELS) + 1;

		ret = slsi_roam_scan_channels_write(dev, command + skip,
						    priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_GETROAMSCANCHANNELS, strlen(CMD_GETROAMSCANCHANNELS)) == 0) {
		ret = slsi_roam_scan_channels_read(dev, priv_cmd.buf, priv_cmd.total_len);
#endif
	} else if (strnicmp(command, CMD_SET_PMK, strlen(CMD_SET_PMK)) == 0) {
		ret = slsi_set_pmk(dev, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_HAPD_GET_CHANNEL, strlen(CMD_HAPD_GET_CHANNEL)) == 0) {
		ret = slsi_auto_chan_read(dev, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SET_SAP_CHANNEL_LIST, strlen(CMD_SET_SAP_CHANNEL_LIST)) == 0) {
		ret = slsi_auto_chan_write(dev, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_REASSOC, strlen(CMD_REASSOC)) == 0) {
		int skip = strlen(CMD_REASSOC) + 1;

		ret = slsi_reassoc_write(dev, command + skip,
					 priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_SENDACTIONFRAME, strlen(CMD_SENDACTIONFRAME)) == 0) {
		int skip = strlen(CMD_SENDACTIONFRAME) + 1;

		ret = slsi_send_action_frame(dev, command + skip,
					     priv_cmd.total_len - skip);
	} else if (strnicmp(command, CMD_HAPD_MAX_NUM_STA, strlen(CMD_HAPD_MAX_NUM_STA)) == 0) {
		int sta_num;
		u8 *max_sta = command + strlen(CMD_HAPD_MAX_NUM_STA) + 1;

		slsi_str_to_int(max_sta, &sta_num);
		ret = slsi_setting_max_sta_write(dev, sta_num);
	} else if (strnicmp(command, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
		char *country_code = command + strlen(CMD_COUNTRY) + 1;

		ret = slsi_country_write(dev, country_code);
	} else if (strnicmp(command, CMD_SEND_GK, strlen(CMD_SEND_GK)) == 0) {
		int skip = strlen(CMD_SEND_GK) + 1;

		ret = slsi_gk_key_write(dev, command + skip,
					priv_cmd.total_len - skip);
#ifdef CONFIG_SCSC_WLAN_OXYGEN_ENABLE
	} else if (strnicmp(command, CMD_SETIBSSBEACONOUIDATA, strlen(CMD_SETIBSSBEACONOUIDATA)) == 0) {
		ret = oxygen_set_ibss_beacon_oui_data(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_GETIBSSPEERINFOALL, strlen(CMD_GETIBSSPEERINFOALL)) == 0) {
		ret = oxygen_get_ibss_peer_info(dev, command, priv_cmd.buf, priv_cmd.total_len, true);
	} else if (strnicmp(command, CMD_GETIBSSPEERINFO, strlen(CMD_GETIBSSPEERINFO)) == 0) {
		ret = oxygen_get_ibss_peer_info(dev, command, priv_cmd.buf, priv_cmd.total_len, false);
	} else if (strnicmp(command, CMD_SETIBSSAMPDU, strlen(CMD_SETIBSSAMPDU)) == 0) {
		ret = oxygen_set_ibss_ampdu(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETIBSSANTENNAMODE, strlen(CMD_SETIBSSANTENNAMODE)) == 0) {
		ret = oxygen_set_ibss_antenna_mode(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETRMCENABLE, strlen(CMD_SETRMCENABLE)) == 0) {
		ret = oxygen_set_rmc_enable(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETRMCTXRATE, strlen(CMD_SETRMCTXRATE)) == 0) {
		ret = oxygen_set_rmc_tx_rate(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETRMCACTIONPERIOD, strlen(CMD_SETRMCACTIONPERIOD)) == 0) {
		ret = oxygen_set_rmc_action_period(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETRMCLEADER, strlen(CMD_SETRMCLEADER)) == 0) {
		ret = oxygen_set_rmc_leader(dev, command, priv_cmd.buf, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETIBSSTXFAILEVENT, strlen(CMD_SETIBSSTXFAILEVENT)) == 0) {
		ret = oxygen_set_ibss_tx_fail_event(dev, command, priv_cmd.buf, priv_cmd.total_len);
#endif /* CONFIG_SCSC_WLAN_OXYGEN_ENABLE */
	} else if (strnicmp(command, CMD_SETAPP2PWPSIE, strlen(CMD_SETAPP2PWPSIE)) == 0) {
		ret = slsi_set_ap_p2p_wps_ie(dev, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_P2PSETPS, strlen(CMD_P2PSETPS)) == 0) {
		ret = slsi_set_p2p_oppps(dev, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_P2PSETNOA, strlen(CMD_P2PSETNOA)) == 0) {
		ret = slsi_p2p_set_noa_params(dev, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_P2PECSA, strlen(CMD_P2PECSA)) == 0) {
		ret = slsi_p2p_ecsa(dev, command);
	} else if (strnicmp(command, CMD_P2PLOSTART, strlen(CMD_P2PLOSTART)) == 0) {
		ret = slsi_p2p_lo_start(dev, command);
	} else if (strnicmp(command, CMD_P2PLOSTOP, strlen(CMD_P2PLOSTOP)) == 0) {
		ret = slsi_p2p_lo_stop(dev);
	} else if (strnicmp(command, CMD_TDLSCHANNELSWITCH, strlen(CMD_TDLSCHANNELSWITCH)) == 0) {
		ret = slsi_tdls_channel_switch(dev, command + strlen(CMD_TDLSCHANNELSWITCH) + 1,
					       priv_cmd.total_len - (strlen(CMD_TDLSCHANNELSWITCH) + 1));
	} else if (strnicmp(command, CMD_SETROAMOFFLOAD, strlen(CMD_SETROAMOFFLOAD)) == 0) {
		ret = slsi_roam_mode_write(dev, command + strlen(CMD_SETROAMOFFLOAD) + 1,
					   priv_cmd.total_len - (strlen(CMD_SETROAMOFFLOAD) + 1));
	} else if (strnicmp(command, CMD_SETROAMOFFLAPLIST, strlen(CMD_SETROAMOFFLAPLIST)) == 0) {
		ret = slsi_roam_offload_ap_list(dev, command + strlen(CMD_SETROAMOFFLAPLIST) + 1,
						priv_cmd.total_len - (strlen(CMD_SETROAMOFFLAPLIST) + 1));
	} else if (strnicmp(command, CMD_SET_TX_POWER_CALLING, strlen(CMD_SET_TX_POWER_CALLING)) == 0) {
		ret = slsi_set_tx_power_calling(dev, command + strlen(CMD_SET_TX_POWER_CALLING) + 1,
						priv_cmd.total_len - (strlen(CMD_SET_TX_POWER_CALLING) + 1));
	} else if (strnicmp(command, CMD_GETREGULATORY, strlen(CMD_GETREGULATORY)) == 0) {
		ret = slsi_get_regulatory(dev, priv_cmd.buf, priv_cmd.total_len);
#ifdef CONFIG_SCSC_WLAN_HANG_TEST
	} else if (strnicmp(command, CMD_TESTFORCEHANG, strlen(CMD_TESTFORCEHANG)) == 0) {
		ret = slsi_test_send_hanged_vendor_event(dev);
#endif
	} else if (strnicmp(command, CMD_SET_FCC_CHANNEL, strlen(CMD_SET_FCC_CHANNEL)) == 0) {
		ret = slsi_set_fcc_channel(dev, command + strlen(CMD_SET_FCC_CHANNEL) + 1,
					   priv_cmd.total_len - (strlen(CMD_SET_FCC_CHANNEL) + 1));
	} else if ((strnicmp(command, CMD_RXFILTERSTART, strlen(CMD_RXFILTERSTART)) == 0) ||
			(strnicmp(command, CMD_RXFILTERSTOP, strlen(CMD_RXFILTERSTOP)) == 0) ||
			(strnicmp(command, CMD_AMPDU_MPDU, strlen(CMD_AMPDU_MPDU)) == 0) ||
			(strnicmp(command, CMD_BTCOEXMODE, strlen(CMD_BTCOEXMODE)) == 0) ||
			(strnicmp(command, CMD_BTCOEXSCAN_START, strlen(CMD_BTCOEXSCAN_START)) == 0) ||
			(strnicmp(command, CMD_BTCOEXSCAN_STOP, strlen(CMD_BTCOEXSCAN_STOP)) == 0) ||
			(strnicmp(command, CMD_CHANGE_RL, strlen(CMD_CHANGE_RL)) == 0) ||
			(strnicmp(command, CMD_INTERFACE_CREATE, strlen(CMD_INTERFACE_CREATE)) == 0) ||
			(strnicmp(command, CMD_INTERFACE_DELETE, strlen(CMD_INTERFACE_DELETE)) == 0) ||
			(strnicmp(command, CMD_LTECOEX, strlen(CMD_LTECOEX)) == 0) ||
			(strnicmp(command, CMD_MIRACAST, strlen(CMD_MIRACAST)) == 0) ||
			(strnicmp(command, CMD_RESTORE_RL, strlen(CMD_RESTORE_RL)) == 0) ||
			(strnicmp(command, CMD_RPSMODE, strlen(CMD_RPSMODE)) == 0) ||
			(strnicmp(command, CMD_SETCCXMODE, strlen(CMD_SETCCXMODE)) == 0) ||
			(strnicmp(command, CMD_SETDFSSCANMODE, strlen(CMD_SETDFSSCANMODE)) == 0) ||
			(strnicmp(command, CMD_SETJOINPREFER, strlen(CMD_SETJOINPREFER)) == 0) ||
			(strnicmp(command, CMD_SETSINGLEANT, strlen(CMD_SETSINGLEANT)) == 0)) {
		SLSI_NET_DBG3(dev, SLSI_CFG80211, "known command %s - no handler\n", command);
		ret  = -ENOTSUPP;
#ifndef SLSI_TEST_DEV
	} else if (strnicmp(command, CMD_DRIVERDEBUGDUMP, strlen(CMD_DRIVERDEBUGDUMP)) == 0) {
		slsi_dump_stats(dev);
		ret = mx140_log_dump();
#endif
	} else {
		SLSI_NET_ERR(dev, "Not Supported : %.*s\n", priv_cmd.total_len, priv_cmd.buf);
		ret  = -ENOTSUPP;
	}

exit:
	kfree(command);
	return ret;
}
