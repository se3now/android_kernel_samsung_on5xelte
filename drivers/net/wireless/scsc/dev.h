/*****************************************************************************
 *
 * Copyright (c) 2012 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __SLSI_DEVICE_H__
#define __SLSI_DEVICE_H__

#include <linux/init.h>
#include <linux/device.h>
#include <linux/inetdevice.h>
#include <net/addrconf.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ratelimit.h>
#include <linux/ip.h>

#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/time.h>
#include <linux/sched.h>

#include <linux/nl80211.h>
#include <linux/wireless.h>
#include <linux/proc_fs.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>
#include <linux/nl80211.h>

#include <scsc/scsc_mx.h>

#include "fapi.h"
#include "const.h"
#include "utils.h"
#include "hip.h"
#include "log_clients.h"
#include "src_sink.h"

#include "scsc_wifi_fcq.h"

#include "scsc_wifi_cm_if.h"
#include "hip4.h"

#ifdef CONFIG_SCSC_WLAN_GSCAN_ENABLE
#include "nl80211_vendor.h"
#endif

#define SLSI_TX_PROCESS_ID_MIN       (0xC001)
#define SLSI_TX_PROCESS_ID_MAX       (0xCF00)
#define SLSI_TX_PROCESS_ID_UDI_MIN   (0xCF01)
#define SLSI_TX_PROCESS_ID_UDI_MAX   (0xCFFE)

/* There are no wakelocks in kernel/supplicant/hostapd.
 * So keep the platform active for some time after receiving any data packet.
 * This timeout value can be fine-tuned based on the test results.
 */
#define SLSI_RX_WAKELOCK_TIME (1000)
#define MAX_BA_BUFFER_SIZE 64
#define NUM_BA_SESSIONS_PER_PEER 8
#define MAX_CHANNEL_LIST 20
#define SLSI_MAX_RX_BA_SESSIONS (8)
#define SLSI_STA_ACTION_FRAME_BITMAP (SLSI_ACTION_FRAME_PUBLIC | SLSI_ACTION_FRAME_WMM | SLSI_ACTION_FRAME_WNM | SLSI_ACTION_FRAME_QOS)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
#define WLAN_EID_VHT_CAPABILITY 191
#define WLAN_EID_VHT_OPERATION 192
#endif

#define NUM_COUNTRY             (300)

#ifdef CONFIG_SCSC_WLAN_MUTEX_DEBUG
#define SLSI_MUTEX_INIT(slsi_mutex__) \
	{ \
		(slsi_mutex__).owner = NULL; \
		mutex_init(&(slsi_mutex__).mutex); \
		(slsi_mutex__).valid = true; \
	}

#define SLSI_MUTEX_LOCK(slsi_mutex_to_lock) \
	{ \
		(slsi_mutex_to_lock).line_no_before = __LINE__; \
		(slsi_mutex_to_lock).file_name_before = __FILE__; \
		mutex_lock(&(slsi_mutex_to_lock).mutex); \
		(slsi_mutex_to_lock).owner = current; \
		(slsi_mutex_to_lock).line_no_after = __LINE__; \
		(slsi_mutex_to_lock).file_name_after = __FILE__; \
		(slsi_mutex_to_lock).function = __func__; \
	}

#define SLSI_MUTEX_UNLOCK(slsi_mutex_to_unlock) \
	{ \
		(slsi_mutex_to_unlock).owner = NULL; \
		mutex_unlock(&(slsi_mutex_to_unlock).mutex); \
	}
#define SLSI_MUTEX_IS_LOCKED(slsi_mutex__) mutex_is_locked(&(slsi_mutex__).mutex)

struct slsi_mutex {
	bool               valid;
	u32                line_no_before;
	const u8           *file_name_before;
	/* a std mutex */
	struct mutex       mutex;
	u32                line_no_after;
	const u8           *file_name_after;
	const u8           *function;
	struct task_struct *owner;
};

#else
#define SLSI_MUTEX_INIT(mutex__)           mutex_init(&(mutex__))
#define SLSI_MUTEX_LOCK(mutex_to_lock)     mutex_lock(&(mutex_to_lock))
#define SLSI_MUTEX_UNLOCK(mutex_to_unlock) mutex_unlock(&(mutex_to_unlock))
#define SLSI_MUTEX_IS_LOCKED(mutex__)      mutex_is_locked(&(mutex__))
#endif

#define OS_UNUSED_PARAMETER(x) ((void)(x))

/* For each mlme-req a mlme-cfm is expected to be received from the
 * firmware. The host is not allowed to send another mlme-req until
 * the mlme-cfm is received.
 *
 * However there are also instances where we need to wait for an mlme-ind
 * following a mlme-req/cfm exchange. One example of this is the disconnect
 * sequence:
 * mlme-disconnect-req - host requests disconnection
 * mlme-disconnect-cfm - firmware accepts disconnection request but hasn't
 *                       disconnected yet.
 * mlme-disconnect-ind - firmware reports final result of disconnection
 *
 * Assuming that waiting for the mlme-ind following on from the mlme-req/cfm
 * is ok.
 */
struct slsi_sig_send {
	/* a std spinlock */
	spinlock_t        send_signal_lock;
#ifdef CONFIG_SCSC_WLAN_MUTEX_DEBUG
	struct slsi_mutex mutex;
#else
	/* a std mutex */
	struct mutex      mutex;
#endif
	struct completion completion;

	u16               process_id;
	u16               req_id;
	u16               cfm_id;
	u16               ind_id;
	struct sk_buff    *cfm;
	struct sk_buff    *ind;
	struct sk_buff    *mib_error;
};

static inline void slsi_sig_send_init(struct slsi_sig_send *sig_send)
{
	spin_lock_init(&sig_send->send_signal_lock);
	sig_send->req_id = 0;
	sig_send->cfm_id = 0;
	sig_send->process_id = SLSI_TX_PROCESS_ID_MIN;
	SLSI_MUTEX_INIT(sig_send->mutex);
	init_completion(&sig_send->completion);
}

struct slsi_ba_frame_desc {
	bool           active;
	struct sk_buff *signal;
	u16            sn;
};

struct slsi_ba_session_rx {
	bool                      active;
	bool                      used;
	void                      *vif;
	struct slsi_ba_frame_desc buffer[MAX_BA_BUFFER_SIZE];
	u16                       buffer_size;
	u16                       occupied_slots;
	u16                       expected_sn;
	u16                       start_sn;
	bool                      trigger_ba_after_ssn;
	u8                        tid;

	/* Aging timer parameters */
	bool                      timer_on;
	struct timer_list         ba_age_timer;
	struct slsi_spinlock      ba_lock;
	struct net_device         *dev;
};

#define SLSI_TID_MAX    (16)
#define SLSI_AMPDU_F_INITIATED      (0x0001)
#define SLSI_AMPDU_F_CREATED        (0x0002)
#define SLSI_AMPDU_F_OPERATIONAL    (0x0004)

#ifdef CONFIG_SCSC_WLAN_RX_NAPI
struct slsi_napi {
	struct napi_struct   napi;
	struct sk_buff_head  rx_data;
	struct slsi_spinlock lock;
	bool                 interrupt_enabled;
};
#endif

#define SLSI_SCAN_HW_ID       0
#define SLSI_SCAN_SCHED_ID    1
#define SLSI_SCAN_MAX         3

/* Per Interface Scan Data
 * Access protected by: cfg80211_lock
 */
struct slsi_scan {
	/* When a Scan is running this not NULL. */
	struct cfg80211_scan_request       *scan_req;
	struct cfg80211_sched_scan_request *sched_req;
	struct sk_buff_head                scan_results;
};

struct slsi_peer {
	/* Flag MUST be set last when creating a record and immediately when removing.
	 * Otherwise another process could test the flag and start using the data.
	 */
	bool                           valid;
	u8                             address[ETH_ALEN];

	/* Presently connected_state is used only for AP/GO mode*/
	u8                             connected_state;
	u16                            aid;
	/* Presently is_wps is used only in P2P GO mode */
	bool                           is_wps;
	u16                            capabilities;
	bool                           qos_enabled;
	u8                             queueset;
	struct scsc_wifi_fcq_data_qset data_qs;
	struct scsc_wifi_fcq_ctrl_q    ctrl_q;

	bool                           authorized;
	bool                           pairwise_key_set;

	/* Needed for STA/AP VIF */
	struct sk_buff                 *assoc_ie;
	struct sk_buff_head            buffered_frames;
	/* Needed for STA VIF */
	struct sk_buff                 *assoc_resp_ie;

	/* bitmask that keeps the status of acm bit for each AC
	 * bit 7  6  5  4  3  2  1  0
	 *     |  |  |  |  |  |  |  |
	 *     vo vo vi vi be bk bk be
	 */
	u8 wmm_acm;
	/* bitmask that keeps the status of tspec establishment for each priority
	 * bit 7  6  5  4  3  2  1  0
	 *     |  |  |  |  |  |  |  |
	 *     p7 p6 p5 p4 p3 p2 p1 p0
	 */
	u8 tspec_established;
	u8 uapsd;

	/* TODO_HARDMAC:
	 * Q: Can we obtain stats from the firmware?
	 *    Yes - then this is NOT needed and we can just get from the firmware when requested.
	 *    No -  How much can we get from the PSCHED?
	 */
	struct station_info       sinfo;
	/* rate limit for peer sinfo mib reads  */
	struct ratelimit_state    sinfo_mib_get_rs;
	struct slsi_ba_session_rx *ba_session_rx[NUM_BA_SESSIONS_PER_PEER];
#ifdef CONFIG_SCSC_WLAN_OXYGEN_ENABLE
	int                       ba_tx_state;
	int                       ba_tx_userpri;
	s8			  signal;
	u16			  txrate;
#endif  /* CONFIG_SCSC_WLAN_OXYGEN_ENABLE */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
	/* qos map configured at peer end*/
	bool	 qos_map_set;
	struct cfg80211_qos_map qos_map;
#endif
};

/* Used to update vif type on vif deactivation indicating vif is no longer available */
#define SLSI_VIFTYPE_UNSPECIFIED   0xFFFF

struct slsi_vif_mgmt_tx {
	u64      cookie;    /* Cookie assigned by Host for the tx mgmt frame */
	u16      host_tag;  /* Host tag for the tx mgmt frame */
	const u8 *buf;      /* Buffer - Mgmt frame requested for tx */
	size_t   buf_len;   /* Buffer length */
	u8       exp_frame; /* Next expected Public action frame subtype from peer */
};

struct slsi_wmm_ac {
	u8  aci_aifsn;
	u8  ecw;
	u16 txop_limit;
}  __packed;

/* struct slsi_wmm_parameter_element
 *
 * eid - Vendor Specific
 * len - Remaining Length of IE
 * oui - Microsoft OUI
 * oui_type - WMM
 * oui_subtype - Param IE
 * version - 1
 * qos_info - Qos
 * reserved -
 * ac - BE,BK,VI,VO
 */
struct slsi_wmm_parameter_element {
	u8                 eid;
	u8                 len;
	u8                 oui[3];
	u8                 oui_type;
	u8                 oui_subtype;
	u8                 version;
	u8                 qos_info;
	u8                 reserved;
	struct slsi_wmm_ac ac[4];
} __packed;

#define SLSI_MIN_FILTER_ID  0x80 /* Start of filter range reserved for host*/

/*for AP*/
#define SLSI_AP_ALL_IPv6_PKTS_FILTER_ID  0x80

/*filter ids for filters installed by driver*/
#ifdef CONFIG_SCSC_WLAN_BLOCK_IPV6

enum slsi_filter_id {
	SLSI_LOCAL_ARP_FILTER_ID = SLSI_MIN_FILTER_ID, /*0x80*/
	SLSI_ALL_BC_MC_FILTER_ID,                         /*0x81*/
	SLSI_PROXY_ARP_FILTER_ID,                      /*0x82*/
	SLSI_ALL_IPv6_PKTS_FILTER_ID,                  /*0x83*/
#ifndef CONFIG_SCSC_WLAN_DISABLE_NAT_KA
	SLSI_NAT_IPSEC_FILTER_ID,                      /*0x84*/
#endif
	SLSI_REGD_MC_FILTER_ID = 0x85
};
#else

/*for STA*/
enum slsi_filter_id {
	SLSI_LOCAL_ARP_FILTER_ID = SLSI_MIN_FILTER_ID, /*0x80*/
	SLSI_ALL_BC_MC_FILTER_ID,                    /*0x81*/
	SLSI_PROXY_ARP_FILTER_ID,                      /*0x82*/
	SLSI_LOCAL_NS_FILTER_ID,                       /*0x83*/
	SLSI_PROXY_ARP_NA_FILTER_ID,                   /*0x84*/
#ifndef CONFIG_SCSC_WLAN_DISABLE_NAT_KA
	SLSI_NAT_IPSEC_FILTER_ID,                      /*0x85*/
#endif
	SLSI_REGD_MC_FILTER_ID = 0x86
};

#endif

#define SLSI_MAX_PKT_FILTERS       16

#ifndef CONFIG_SCSC_WLAN_DISABLE_NAT_KA
/*default config*/
#define SLSI_MC_ADDR_ENTRY_MAX  (SLSI_MIN_FILTER_ID + SLSI_MAX_PKT_FILTERS - SLSI_REGD_MC_FILTER_ID)
#else
#define SLSI_MC_ADDR_ENTRY_MAX  (SLSI_MIN_FILTER_ID + SLSI_MAX_PKT_FILTERS - SLSI_REGD_MC_FILTER_ID + 1)
#endif

/* Values for vif_status field
 *
 * Used to indicate the status of an activated VIF, to help resolve
 * conflicting activities with indications from the firmware eg.
 * cfg80211 triggers a disconnection before a STA completes its
 * connection to an AP.
 */
#define SLSI_VIF_STATUS_UNSPECIFIED   0
#define SLSI_VIF_STATUS_CONNECTING    1
#define SLSI_VIF_STATUS_CONNECTED     2
#define SLSI_VIF_STATUS_DISCONNECTING 3

/*From wifi_offload.h (N_AVAIL_ID=3)*/
#define SLSI_MAX_KEEPALIVE_ID 3

struct slsi_vif_sta {
	/* Only valid when the VIF is activated */
	u8                          vif_status;
	struct delayed_work     roam_report_work;
	bool                    is_wps;
	u16                     eap_hosttag;
	u16                     m4_host_tag;
	u16                     keepalive_host_tag[SLSI_MAX_KEEPALIVE_ID];
	u8                      mac_addr[ETH_ALEN];

	struct sk_buff          *roam_mlme_procedure_started_ind;

	/* This id is used to find out which response  (connect resp/roamed resp/reassoc resp)
	 * is to be sent once M4 is transmitted successfully
	 */
	u16                     resp_id;
	bool                    gratuitous_arp_needed;

	/* regd multicast address*/
	u8                      regd_mc_addr_count;
	u8                      regd_mc_addr[SLSI_MC_ADDR_ENTRY_MAX][ETH_ALEN];
	bool                    group_key_set;
	struct sk_buff          *mlme_scan_ind_skb;
	bool                    roam_in_progress;
	int                     tdls_peer_sta_records;
	bool                    tdls_enabled;
	struct cfg80211_bss     *sta_bss;

	/* List of seen ESS and Freq associated with them */
	struct list_head        network_map;
};

struct slsi_vif_unsync {
	struct delayed_work roc_expiry_work;   /* Work on ROC duration expiry */
	struct delayed_work del_vif_work;      /* Work on unsync vif retention timeout */
	struct delayed_work hs2_del_vif_work;  /* Work on HS2 unsync vif retention timeout */
	u64                 roc_cookie;        /* Cookie id for ROC */
	u8                  *probe_rsp_ies;    /* Probe response IEs to be configured in firmware */
	size_t              probe_rsp_ies_len; /* Probe response IE length */
	bool                ies_changed;       /* To indicate if Probe Response IEs have changed from that previously stored */
	bool                listen_offload;    /* To indicate if Listen Offload is started */
};

struct slsi_vif_ap {
	struct slsi_wmm_parameter_element wmm_ie;
	u8                                *cache_wmm_ie;
	u8                                *cache_wpa_ie;
	u8                                *add_info_ies;
	size_t                            wmm_ie_len;
	size_t                            wpa_ie_len;
	size_t                            add_info_ies_len;
	bool                              p2p_gc_keys_set;       /* Used in GO mode to identify that a CLI has connected after WPA2 handshake */
	bool                              privacy;               /* Used for port enabling based on the open/secured AP configuration */
	int                               beacon_interval;       /* Beacon interval in AP/GO mode */
	bool                              non_ht_bss_present;    /* Non HT BSS observed in HT20 OBSS scan */
	struct scsc_wifi_fcq_data_qset    group_data_qs;
	u32                               cipher;
};

#ifdef CONFIG_SCSC_WLAN_OXYGEN_ENABLE
struct slsi_vif_ibss {
	u8     *cache_wmm_param_ie;
	u8     *cache_wmm_info_ie;
	u8     *cache_wpa_ie;
	u8     *cache_oui_ie;
	u8     *add_info_ies;
	size_t wmm_param_ie_len;
	size_t wmm_info_ie_len;
	size_t wpa_ie_len;
	size_t oui_ie_len;
	size_t add_info_ies_len;
	int    beacon_interval;
	int    rmc_txrate;
	u8     rmc_addr[ETH_ALEN];
	int    ba_tx_userpri;
	int    olsrd_pid;
	int    tx_fail_seq;
};
#endif

struct netdev_vif {
	struct slsi_dev             *sdev;
	struct wireless_dev         wdev;
	atomic_t                    is_registered;         /* Has the net dev been registered */
	bool                        is_available;          /* Has the net dev been opened AND is usable */
	bool                        is_fw_test;            /* Is the device in use as a test device via UDI */

	/* Structure can be accessed by cfg80211 ops, procfs/ioctls and as a result
	 * of receiving MLME indications e.g. MLME-CONNECT-IND that can affect the
	 * status of the interface eg. STA connect failure will delete the VIF.
	 */
#ifdef CONFIG_SCSC_WLAN_MUTEX_DEBUG
	struct slsi_mutex           vif_mutex;
#else
	/* a std mutex */
	struct mutex                vif_mutex;
#endif
	struct slsi_sig_send        sig_wait;
	struct slsi_skb_work        rx_data;
	struct slsi_skb_work        rx_mlme;
#ifdef CONFIG_SCSC_WLAN_RX_NAPI
	struct slsi_napi            napi;
#endif
	u16                         ifnum;
	enum nl80211_iftype         iftype;
	enum nl80211_channel_type   channel_type;
	struct ieee80211_channel    *chan;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 9))
	struct cfg80211_chan_def    *chandef;
#endif

	/* NOTE: The Address is a __be32
	 * It needs converting to pass to the FW
	 * But not for the Arp or trace %pI4
	 */
	__be32                      ipaddress;

#ifndef CONFIG_SCSC_WLAN_BLOCK_IPV6
	struct in6_addr             ipv6address;
	struct slsi_spinlock        ipv6addr_lock;
#endif
	struct net_device_stats     stats;
#ifdef CONFIG_SCSC_WLAN_MUTEX_DEBUG
	struct slsi_mutex           scan_mutex;
#else
	/* a std mutex */
	struct mutex                scan_mutex;
#endif
	struct slsi_scan            scan[SLSI_SCAN_MAX];

	struct slsi_src_sink_params src_sink_params;
	u16                         power_mode;
	u16                         set_power_mode;

	bool                        activated;      /* VIF is created in firmware and ready to use */
	u16                         vif_type;
	struct slsi_spinlock        peer_lock;
	int                         peer_sta_records;
	struct slsi_peer            *peer_sta_record[SLSI_ADHOC_PEER_CONNECTIONS_MAX];

	/* Used to populate the cfg80211 station_info structure generation variable.
	 * This number should increase every time the list of stations changes
	 * i.e. when a station is added or removed, so that userspace can tell
	 * whether it got a consistent snapshot.
	 */
	int                         cfg80211_sinfo_generation;

	/* Block Ack MPDU Re-order */
	struct sk_buff_head         ba_complete;
	atomic_t                    ba_flush;

	u64                         mgmt_tx_cookie; /* Cookie id for mgmt tx */
	struct slsi_vif_mgmt_tx     mgmt_tx_data;
	struct delayed_work         scan_timeout_work;     /* Work on scan timeout */

	struct slsi_vif_unsync      unsync;
	struct slsi_vif_sta         sta;
	struct slsi_vif_ap          ap;
#ifdef CONFIG_SCSC_WLAN_OXYGEN_ENABLE
	struct slsi_vif_ibss	    ibss;
#endif
};

struct slsi_802_11d_reg_domain {
	u8                         alpha2[3];
	u8                         *countrylist;
	struct ieee80211_regdomain *regdomain;
	int                        country_len;
};

#ifdef CONFIG_SCSC_WLAN_WES_NCHO
struct slsi_wes_mode_roam_scan_channels {
	int n;
	u8  channels[MAX_CHANNEL_LIST];
};
#endif

struct slsi_dev_config {
	/* Current Channel Config */
	struct sk_buff *channel_config;

	/* Supported Freq Band (Dynamic)
	 * Set via the freq_band procfs
	 */
#define SLSI_FREQ_BAND_AUTO 0
#define SLSI_FREQ_BAND_5GHZ 1
#define SLSI_FREQ_BAND_2GHZ 2
	int                             supported_band;

	struct ieee80211_supported_band *band_5G;
	struct ieee80211_supported_band *band_2G;

	/* current user suspend mode
	 * Set via the suspend_mode procfs
	 * 0 : not suspended
	 * 1 : suspended
	 */
	int                                     user_suspend_mode;

	/* Rx filtering rule
	 * Set via the rx_filter_num procfs
	 * 0: Unicast, 1: Broadcast, 2:Multicast IPv4, 3: Multicast IPv6
	 */
	int                                     rx_filter_num;

	/* Rx filter rule enabled
	 * Set  via the rx_filter_start & rx_filter_stop procfs
	 */
	bool                                    rx_filter_rule_started;

	/* Read from FW mib at startup */
	unsigned int                            fast_power_save_timeout;

	/* AP Auto channel Selection */
#define SLSI_NO_OF_SCAN_CHANLS_FOR_AUTO_CHAN_MAX 14
	int                                     ap_auto_chan;

	/*QoS capability for a non-AP Station*/
	int                                     qos_info;
#ifdef CONFIG_SCSC_WLAN_WES_NCHO
	/* NCHO OKC mode */
	int                                     okc_mode;

	/*NCHO WES mode */
	int                                     wes_mode;

	int                                     roam_scan_mode;

	/*WES mode roam scan channels*/
	struct slsi_wes_mode_roam_scan_channels wes_roam_scan_list;
#endif
	struct slsi_802_11d_reg_domain          domain_info;

	int                                     ap_disconnect_ind_timeout;
};

#define SLSI_DEVICE_STATE_ATTACHING 0
#define SLSI_DEVICE_STATE_STOPPED   1
#define SLSI_DEVICE_STATE_STARTING  2
#define SLSI_DEVICE_STATE_STARTED   3
#define SLSI_DEVICE_STATE_STOPPING  4

#define SLSI_NET_INDEX_WLAN 1
#define SLSI_NET_INDEX_P2P  2
#define SLSI_NET_INDEX_P2PX 3

/* States used during P2P operations */
enum slsi_p2p_states {
	P2P_IDLE_NO_VIF,        /* Initial state - Unsync vif is not present */
	P2P_IDLE_VIF_ACTIVE,    /* Unsync vif is present but no P2P procedure in progress */
	P2P_SCANNING,           /* P2P SOCIAL channel (1,6,11) scan in progress. Not used for P2P full scan */
	P2P_LISTENING,          /* P2P Listen (ROC) in progress */
	P2P_ACTION_FRAME_TX_RX, /* P2P Action frame Tx in progress or waiting for a peer action frame Rx (i.e. in response to the Tx frame) */
	P2P_GROUP_FORMED_CLI,   /* P2P Group Formed - CLI role */
	P2P_GROUP_FORMED_GO,    /* P2P Group Formed - GO role */
	/* NOTE: In P2P_LISTENING state if frame transmission is requested to driver then a peer response is ideally NOT expected.
	 * This is an assumption based on the fact that FIND would be stopped prior to group formation/connection.
	 * If driver were to receive a peer frame in P2P_LISTENING state then it would most probably be a REQUEST frame and the supplicant would respond to it.
	 * Hence the driver should get only RESPONSE frames for transmission in P2P_LISTENING state.
	 */
};

enum slsi_hs2_state {
	HS2_NO_VIF = 0,           /* Initial state - Unsync vif is not present */
	HS2_VIF_ACTIVE,           /* Unsync vif is activated but no HS procedure in progress */
	HS2_VIF_TX                /* Unsync vif is activated and HS procedure in progress */
};

/* Wakelock timeouts */
#define SLSI_WAKELOCK_TIME_MSEC_EAPOL (1000)

struct slsi_chip_info_mib {
	u16 chip_version;
	u32 fw_build_id;
	u32 fw_patch_id;
};

/* P2P States in text format for debug purposes */
static inline char *slsi_p2p_state_text(u8 state)
{
	switch (state) {
	case P2P_IDLE_NO_VIF:
		return "P2P_IDLE_NO_VIF";
	case P2P_IDLE_VIF_ACTIVE:
		return "P2P_IDLE_VIF_ACTIVE";
	case P2P_SCANNING:
		return "P2P_SCANNING";
	case P2P_LISTENING:
		return "P2P_LISTENING";
	case P2P_ACTION_FRAME_TX_RX:
		return "P2P_ACTION_FRAME_TX_RX";
	case P2P_GROUP_FORMED_CLI:
		return "P2P_GROUP_FORMED_CLI";
	case P2P_GROUP_FORMED_GO:
		return "P2P_GROUP_FORMED_GO";
	default:
		return "UNKNOWN";
	}
}

struct hydra_service_info {
	char     ver_str[128];
	uint16_t hw_ver;
	uint32_t fw_rom_ver;
	uint32_t fw_patch_ver;
	bool     populated;
};

struct slsi_dev {
	/* Devices */
	struct device              *dev;
	struct wiphy               *wiphy;

	struct slsi_hip            hip;       /* HIP bookkeeping block */
	struct slsi_hip4           hip4_inst; /* The handler to parse to HIP */

	struct scsc_wifi_cm_if     cm_if;     /* cm_if bookkeeping block */
	struct scsc_mx             *maxwell_core;
	struct scsc_service_client mx_wlan_client;
	struct scsc_service        *service;

	struct hydra_service_info  chip_info;
	struct slsi_chip_info_mib  chip_info_mib;
#ifdef CONFIG_SCSC_WLAN_MUTEX_DEBUG
	struct slsi_mutex          netdev_add_remove_mutex;
#else
	/* a std mutex */
	struct mutex               netdev_add_remove_mutex;
#endif
	int                        netdev_up_count;
	struct net_device          __rcu *netdev[CONFIG_SCSC_WLAN_MAX_INTERFACES + 1];               /* 0 is reserved */
	u8                         netdev_addresses[CONFIG_SCSC_WLAN_MAX_INTERFACES + 1][ETH_ALEN];  /* 0 is reserved */

	int                        device_state;

	/* Locking used to control Starting and stopping the chip */
#ifdef CONFIG_SCSC_WLAN_MUTEX_DEBUG
	struct slsi_mutex          start_stop_mutex;
#else
	/* a std mutex */
	struct mutex               start_stop_mutex;
#endif

#ifdef CONFIG_SCSC_WLAN_OFFLINE_TRACE
	struct slsi_spinlock       offline_dbg_lock;
#endif

	/* UDI Logging */
	struct slsi_log_clients    log_clients;
	void                       *uf_cdev;

	/* ProcFS */
	int                        procfs_instance;
	struct proc_dir_entry      *procfs_dir;

	/* Configuration */
	u8                         hw_addr[ETH_ALEN];
	char                       *mib_file_name;
	char                       *local_mib_file_name;
	char                       *maddr_file_name;
	bool                       *term_udi_users;   /* Try to terminate UDI users during unload */
	int                        *sig_wait_cfm_timeout;

	/* Cached File MIB Configuration values from User Space */
	u8                         *mib_data;
	u32                         mib_len;

	struct slsi_wake_lock      wlan_wl;
	struct slsi_wake_lock      wlan_wl_to;
#ifndef SLSI_TEST_DEV
	struct wake_lock           wlan_roam_wl;
#endif
	struct slsi_sig_send       sig_wait;
	struct slsi_skb_work       rx_dbg_sap;
	atomic_t                   tx_host_tag;
#ifdef CONFIG_SCSC_WLAN_MUTEX_DEBUG
	struct slsi_mutex          device_config_mutex;
#else
	/* a std mutex */
	struct mutex               device_config_mutex;
#endif
	struct slsi_dev_config     device_config;

	struct notifier_block      inetaddr_notifier;
#ifndef CONFIG_SCSC_WLAN_BLOCK_IPV6
	struct notifier_block      inet6addr_notifier;
#endif

	struct workqueue_struct    *device_wq;              /* Driver Workqueue */
	enum slsi_p2p_states       p2p_state;               /* Store current P2P operation */
	bool                       delete_gc_probe_req_ies; /* Delete probe request stored at  gc_probe_req_ies, if connected for WAP2 at mlme_del_vif */
	u8                         *gc_probe_req_ies;
	size_t                     gc_probe_req_ie_len;

	enum slsi_hs2_state        hs2_state; /* Store current HS2 operations */

	int                        current_tspec_id;
	int                        tspec_error_code;
	bool                       *tx_cfm_reqd;
	u8                         p2p_group_exp_frame;        /* Next expected Public action frame subtype from peer */
	bool                       scan_init_24g;

#ifdef CONFIG_SCSC_WLAN_GSCAN_ENABLE
	struct slsi_gscan          *gscan;
	struct slsi_gscan_result   *gscan_hash_table[SLSI_GSCAN_HASH_TABLE_SIZE];
	int                        num_gscan_results;
	int                        buffer_threshold;
	int                        buffer_consumed;
	struct slsi_bucket         bucket[SLSI_GSCAN_MAX_BUCKETS];
	struct list_head           hotlist_results;
	bool                       epno_active;
#endif
#ifdef CONFIG_SCSC_WLAN_DEBUG
	int                        minor_prof;
#endif
	struct slsi_ba_session_rx  rx_ba_buffer_pool[SLSI_MAX_RX_BA_SESSIONS];
	struct slsi_spinlock       rx_ba_buffer_pool_lock;
	bool			   fail_reported;
	bool			   p2p_certif; /* Set to true to idenitfy p2p_certification testing is going on*/
	bool                       mlme_blocked; /* When true do not send mlme signals to FW */
	atomic_t		   debug_inds;
	int                        recovery_next_state;
	struct completion          recovery_remove_completion;
	struct completion          recovery_stop_completion;
	int                        recovery_status;
};

/* Compact representation of channels a ESS has been seen on
 * This is sized correctly for the Channels we currently support,
 * 2.4Ghz Channels 1 - 14
 * 5  Ghz Channels Uni1, Uni2 and Uni3
 */
struct slsi_roaming_network_map_entry {
	struct list_head     list;
	unsigned long        last_seen_jiffies;       /* Timestamp of the last time we saw this ESS */
	struct cfg80211_ssid ssid;                    /* SSID of the ESS */
	u8                   initial_bssid[ETH_ALEN]; /* Bssid of the first ap seen in this ESS */
	bool                 only_one_ap_seen;        /* Has more than one AP for this ESS been seen */
	u16                  channels24Ghz;           /* 2.4 Ghz Channels Bit Map*/
	/* 5 Ghz Channels Bit Map
	 * channels5Ghz & 0x000000FF =  4 Uni1 Channels
	 * channels5Ghz & 0x00FFFF00 = 15 Uni2 Channels
	 * channels5Ghz & 0xFF000000 =  5 Uni3 Channels
	 */
	u32                  channels5Ghz;
};

#define LLC_SNAP_HDR_LEN 8
struct llc_snap_hdr {
	u8 llc_dsap;
	u8 llc_ssap;
	u8 llc_ctrl;
	u8 snap_oui[3];
	u16 snap_type;
} __packed;

#ifdef CONFIG_SCSC_WLAN_RX_NAPI
int slsi_rx_data_napi(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb, bool fromBA);
#endif
int slsi_rx_data(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb, bool fromBA);
void slsi_rx_dbg_sap_work(struct work_struct *work);
void slsi_rx_netdev_data_work(struct work_struct *work);
void slsi_rx_netdev_mlme_work(struct work_struct *work);
int slsi_rx_enqueue_netdev_mlme(struct slsi_dev *sdev, struct sk_buff *skb, u16 vif);
void slsi_rx_scan_pass_to_cfg80211(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_buffered_frames(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *peer);
int slsi_rx_blocking_signals(struct slsi_dev *sdev, struct sk_buff *skb);
void slsi_scan_complete(struct slsi_dev *sdev, struct net_device *dev, u16 scan_id, bool aborted);

void slsi_tx_pause_queues(struct slsi_dev *sdev);
void slsi_tx_unpause_queues(struct slsi_dev *sdev);
int slsi_tx_control(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
int slsi_tx_data(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
int slsi_tx_data_lower(struct slsi_dev *sdev, struct sk_buff *skb);
bool slsi_is_wlan_service_active(void);
bool slsi_is_test_mode_enabled(void);
bool slsi_is_232338_test_mode_enabled(void);
int check_mcd_232338_rf_mode(void);
void slsi_regd_deinit(struct slsi_dev *sdev);
void slsi_init_netdev_mac_addr(struct slsi_dev *sdev);

static inline u16 slsi_tx_host_tag(struct slsi_dev *sdev)
{
	return (u16)atomic_inc_return(&sdev->tx_host_tag);
}

static inline struct net_device *slsi_get_netdev_rcu(struct slsi_dev *sdev, u16 ifnum)
{
	WARN_ON(!rcu_read_lock_held());
	if (ifnum > CONFIG_SCSC_WLAN_MAX_INTERFACES) {
		WARN(1, "ifnum:%d", ifnum); /* WARN() is used like this to avoid Coverity Error */
		return NULL;
	}
	return rcu_dereference(sdev->netdev[ifnum]);
}

static inline struct net_device *slsi_get_netdev_locked(struct slsi_dev *sdev, u16 ifnum)
{
	WARN_ON(!SLSI_MUTEX_IS_LOCKED(sdev->netdev_add_remove_mutex));
	if (ifnum > CONFIG_SCSC_WLAN_MAX_INTERFACES) {
		WARN(1, "ifnum:%d", ifnum); /* WARN() is used like this to avoid Coverity Error */
		return NULL;
	}
	return sdev->netdev[ifnum];
}

static inline struct net_device *slsi_get_netdev(struct slsi_dev *sdev, u16 ifnum)
{
	struct net_device *dev;

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	dev = slsi_get_netdev_locked(sdev, ifnum);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);

	return dev;
}

#ifdef CONFIG_SCSC_WLAN_OXYGEN_ENABLE
static inline void slsi_ibss_ampdu_setmode(struct slsi_peer *peer, int mode)
{
	if (~peer->ba_tx_state & mode)
		peer->ba_tx_state |= mode;
}

static inline void slsi_ibss_ampdu_clrmode(struct slsi_peer *peer, int mode)
{
	if (peer->ba_tx_state & mode)
		peer->ba_tx_state &= ~mode;
}
#endif  /* CONFIG_SCSC_WLAN_OXYGEN_ENABLE */

#endif
