/*
 	bpP.h:	private definitions supporting the implementation
		of BP (Bundle Protocol) nodes in the ION
		(Interplanetary Overlay Network) stack, including
		scheme-specific forwarders and the convergence-layer
		adapters they rely on.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _BPP_H_
#define _BPP_H_

#include "rfx.h"
#include "ionsec.h"
#include "bp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	"Watch" switches for bundle protocol operation.			*/
#define	WATCH_a				(1)
#define	WATCH_b				(2)
#define	WATCH_c				(4)
#define	WATCH_m				(8)
#define	WATCH_w				(16)
#define	WATCH_x				(32)
#define	WATCH_y				(64)
#define	WATCH_z				(128)
#define	WATCH_abandon			(256)
#define	WATCH_expire			(512)
#define	WATCH_refusal			(1024)
#define	WATCH_timeout			(2048)
#define	WATCH_limbo			(4096)
#define	WATCH_delimbo			(8192)

#define SDR_LIST_ELT_OVERHEAD		(WORD_SIZE * 4)

/*	A note on coding technique: the checking of return codes
 *	can safely be foregone in some circumstances, to simplify
 *	the code and marginally improve performance.  The general
 *	rule is that while an SDR transaction is open it is not
 *	necessary to check the value returned by an SDR function
 *	UNLESS the result of that function must be non-failure
 *	in order for subsequent operations in the scope of the
 *	same transaction to be safe [e.g., to prevent segmentation
 *	faults].  The reason for this is that sdr_end_xn(), when
 *	it is invoked to conclude the transaction, will itself
 *	fail and back out the transaction if any SDR operation
 *	within the scope of the transaction failed; it is only
 *	(and ALWAYS) necessary to check the value returned by
 *	sdr_end_xn().  Note also that, if you do check the value
 *	returned by an SDR function while a transaction is open
 *	and it indicates failure, it is NOT necessary to abort
 *	the transaction by calling sdr_cancel_xn(); the transaction
 *	has already been aborted.  It is only necessary to return
 *	a failure indication to invoking code and let sdr_end_xn()
 *	at the top of the function stack clean up the transaction
 *	and return -1 to trigger the applicable failure handling.	*/

#define MIN_PRIMARY_BLK_LENGTH		(23)
#define MAX_CL_PROTOCOL_NAME_LEN	(15)
#define MAX_CL_DUCT_NAME_LEN		(255)
#define	MAX_SCHEME_NAME_LEN		(15)
#define	MAX_NSS_LEN			(63)
#define	MAX_EID_LEN			(MAX_SCHEME_NAME_LEN + MAX_NSS_LEN + 2)
#define MAX_CBHE_NODE_NBR		(16777215)
#define MAX_CBHE_SERVICE_NBR		(32767)

#ifndef	BP_MAX_BLOCK_SIZE
#define BP_MAX_BLOCK_SIZE		(2000)
#endif

/*	We hitchhike on the ZCO heap space management system to 
 *	manage the space occupied by Bundle objects.  In effect,
 *	the Bundle overhead objects compete with ZCOs for available
 *	SDR heap space.  We don't want this practice to become
 *	widespread, which is why these functions are declared
 *	privately here rather than publicly in the zco.h header.	*/

extern void	zco_increase_heap_occupancy(Sdr sdr, vast delta);
extern void	zco_reduce_heap_occupancy(Sdr sdr, vast delta);

/*	A BP "node" is a set of cooperating state machines that
 *	together constitute a single functional point of presence,
 *	residing in a single SDR database, in a DTN-based network.
 *
 *	A single node may be equipped to send and receive bundles
 *	to and from endpoints whose IDs are formed under one or
 *	more "schemes".  For each such scheme the node will have
 *	a single scheme-specific "forwarder", for which scheme-
 *	specific forwarding information may be recorded in the
 *	database.
 *
 *	To utilize the services of the node, an application instance
 *	calls bp_open() to reserve for its own use a specified DTN
 *	endpoint that is registered at this node, and then calls
 *	bp_receive() to extract inbound bundles from the queue of
 *	undelivered inbound bundles for that endpoint and/or calls
 *	bp_send() to issue bundles to some specified endpoint.
 *
 *	The blocks of data that DTN applications exchange among
 *	themselves are termed "application data units" (ADUs).  A
 *	BP forwarder packages ADUs within bundles and then uses
 *	services provided by underlying communication protocols (at
 *	the "convergence layer" and below) to transmit the bundles
 *	to other nodes.							*/

typedef struct
{
	uvast		nbr;
} NodeId;

typedef struct
{
	sm_SemId	semaphore;
	int		nominalRate;	/*	In bytes per second.	*/
	vast		capacity;	/*	Bytes, current second.	*/
} Throttle;

typedef struct
{
	Object		text;		/*	Not NULL-terminated.	*/
	unsigned int	textLength;
} BpString;

typedef struct
{
	unsigned short	schemeNameOffset;	/*	In dictionary.	*/
	unsigned short	nssOffset;		/*	In dictionary.	*/
} DtnEid;

typedef struct
{
	uvast		nodeNbr;	/*	For "imc", group nbr.	*/
	unsigned int	serviceNbr;
} CbheEid;

typedef struct
{
	DtnEid		d;
	CbheEid		c;
	char		cbhe;		/*	Boolean.		*/
	char		unicast;	/*	Boolean.		*/
} EndpointId;

typedef struct
{
	uvast		nodeNbr;
	unsigned int	serviceNbr;
	char		*schemeName;
	int		schemeNameLength;
	char		*colon;
	char		*nss;
	int		nssLength;
	char		cbhe;		/*	Boolean.		*/
	char		nullEndpoint;	/*	Boolean.		*/
} MetaEid;

typedef struct
{
	char		*protocolName;
	char		*proxNodeEid;
} DequeueContext;

/*	*	*	Bundle structures	*	*	*	*/

/*	For non-fragmentary bundles, and for the first fragmentary
 *	bundle of a fragmented source bundle, fragmentOffset is zero.
 *	For fragmentary bundles other than the first, fragmentOffset
 *	indicates the offset of the fragmentary bundle's payload
 *	from the beginning of the payload of the original source
 *	bundle.								*/

typedef struct
{
	EndpointId	source;		/*	Original sender.	*/
	BpTimestamp	creationTime;
	unsigned int	fragmentOffset;
} BundleId;

typedef struct
{
	unsigned int	length;		/*	initial length of ZCO	*/
	Object		content;	/*	a ZCO reference in SDR	*/
} Payload;

/*	Administrative record types	*/
#define	BP_STATUS_REPORT	(1)
#define	BP_CUSTODY_SIGNAL	(2)

/*	Administrative record flags	*/
#define BP_BDL_IS_A_FRAGMENT	(1)	/*	00000001		*/

typedef enum
{
	SrLifetimeExpired = 1,
	SrUnidirectionalLink,
	SrCanceled,
	SrDepletedStorage,
	SrDestinationUnintelligible,
	SrNoKnownRoute,
	SrNoTimelyContact,
	SrBlockUnintelligible
} BpSrReason;

typedef struct
{
	unsigned int	seconds;
	unsigned int	nanosec;
} DtnTime;

typedef struct
{
	BpTimestamp	creationTime;	/*	From bundle's ID.	*/
	unsigned int	fragmentOffset;	/*	From bundle's ID.	*/
	unsigned int	fragmentLength;
	char		*sourceEid;	/*	From bundle's ID.	*/
	unsigned char	isFragment;	/*	Boolean.		*/
	unsigned char	flags;
	BpSrReason	reasonCode;
	DtnTime		receiptTime;
	DtnTime		acceptanceTime;
	DtnTime		forwardTime;
	DtnTime		deliveryTime;
	DtnTime		deletionTime;
} BpStatusRpt;

typedef enum
{
	CtRedundantReception = 3,
	CtDepletedStorage,
	CtDestinationUnintelligible,
	CtNoKnownRoute,
	CtNoTimelyContact,
	CtBlockUnintelligible
} BpCtReason;

typedef struct
{
	BpTimestamp	creationTime;	/*	From bundle's ID.	*/
	unsigned int	fragmentOffset;	/*	From bundle's ID.	*/
	unsigned int	fragmentLength;
	char		*sourceEid;	/*	From bundle's ID.	*/
	unsigned char	isFragment;	/*	Boolean.		*/
	unsigned char	succeeded;	/*	Boolean.		*/
	BpCtReason	reasonCode;
	DtnTime		signalTime;
} BpCtSignal;

/*	The convergence-layer adapter uses the ClDossier structure to
 *	assert its own private knowledge about the bundle: authenticity
 *	and/or EID of sender.						*/

typedef struct
{
	int		authentic;	/*	Boolean.		*/
	BpString	senderEid;
	uvast		senderNodeNbr;	/*	If CBHE.		*/
} ClDossier;

/*	Bundle processing flags						*/
#define BDL_IS_FRAGMENT		(1)	/*	00000001		*/
#define BDL_IS_ADMIN		(2)	/*	00000010		*/
#define BDL_DOES_NOT_FRAGMENT	(4)	/*	00000100		*/
#define BDL_IS_CUSTODIAL	(8)	/*	00001000		*/
#define BDL_DEST_IS_SINGLETON	(16)	/*	00010000		*/
#define BDL_APP_ACK_REQUEST	(32)	/*	00100000		*/

/*	Block processing flags						*/
#define BLK_MUST_BE_COPIED	(1)	/*	00000001		*/
#define BLK_REPORT_IF_NG	(2)	/*	00000010		*/
#define BLK_ABORT_IF_NG		(4)	/*	00000100		*/
#define BLK_IS_LAST		(8)	/*	00001000		*/
#define BLK_REMOVE_IF_NG	(16)	/*	00010000		*/
#define BLK_FORWARDED_OPAQUE	(32)	/*	00100000		*/
#define BLK_HAS_EID_REFERENCES	(64)	/*	01000000		*/

/*	Extension locations						*/
#define PRE_PAYLOAD		(0)
#define POST_PAYLOAD		(1)

typedef struct
{
	BundleId	id;

	/*	Stuff in Primary block.					*/

	unsigned int	bundleProcFlags;/*	Incl. CoS, SRR.		*/
	unsigned int	timeToLive;	/*	In seconds.		*/
	EndpointId	destination;	/*	...of bundle's ADU	*/
		/*	source of bundle's ADU is in the id field.	*/
	EndpointId	reportTo;
	EndpointId	custodian;
		/*	creation time is in the id field.		*/
	unsigned int	expirationTime;	/*	Time tag in seconds.	*/
	unsigned int	dictionaryLength;
	Object		dictionary;
		/*	fragment offset is in the id field.		*/
	unsigned int	totalAduLength;

	/*	Stuff in Extended COS extension block.			*/

	BpExtendedCOS	extendedCOS;

	/*	Stuff in (or for) the Bundle Age extension block.	*/

	unsigned int	age;		/*	In microseconds.	*/
	struct timeval	arrivalTime;

	/*	Stuff in Payload block.					*/

	unsigned int	payloadBlockProcFlags;
	Payload		payload;

	/*	Stuff in extension blocks, preceding and following the
	 *	payload block: SDR lists of ExtensionBlock objects.
	 *	Each extensionsLength field is the sum of the "length"
	 *	values of all ExtensionBlocks in the corresponding
	 *	extensions list.					*/

	Object		extensions[2];
	int		extensionsLength[2];	/*	Concatenated.	*/
	Object		collabBlocks;	/*	SDR list of C. blocks.	*/

	/*	Internal housekeeping stuff.				*/

	char		custodyTaken;	/*	Boolean.		*/
	char		delivered;	/*	Boolean.		*/
	char		suspended;	/*	Boolean.		*/
	char		returnToSender;	/*	Boolean.		*/
	char		accepted;	/*	Boolean.		*/
	char		corrupt;	/*	Boolean.		*/
	char		anonymous;	/*	Boolean.		*/
	int		dbOverhead;	/*	SDR bytes occupied.	*/
	BpStatusRpt	statusRpt;	/*	For response per CoS.	*/
	BpCtSignal	ctSignal;	/*	For acknowledgement.	*/
	ClDossier	clDossier;	/*	Processing hints.	*/
	Object		stations;	/*	Stack of EIDs (route).	*/

	/*	Database navigation stuff (back-references).		*/

	Object		hashEntry;	/*	Entry in bundles hash.	*/

	Object		timelineElt;	/*	TTL expire list ref.	*/
	Object		overdueElt;	/*	Xmit overdue ref.	*/
	Object		ctDueElt;	/*	CT deadline ref.	*/
	Object		fwdQueueElt;	/*	Scheme's queue ref.	*/
	Object		fragmentElt;	/*	Incomplete's list ref.	*/
	Object		dlvQueueElt;	/*	Endpoint's queue ref.	*/
	Object		trackingElts;	/*	List of app. list refs.	*/

	Object		incompleteElt;	/*	Ref. to Incomplete.	*/

	/*	Transmission queue (or limbo list) stuff.		*/

	Object		ductXmitElt;	/*	Transmission queue ref.	*/
	Object		proxNodeEid;	/*	An SDR string		*/
	Object		destDuctName;	/*	An SDR string		*/
	time_t		enqueueTime;	/*	When queued for xmit.	*/
} Bundle;

#define COS_FLAGS(bundleProcFlags)	((bundleProcFlags >> 7) & 0x7f)
#define SRR_FLAGS(bundleProcFlags)	((bundleProcFlags >> 14) & 0x7f)

typedef struct
{
	Object		fragments;	/*	SDR list		*/

	/*	Each element of the fragments list is a Bundle object;
	 *	the list is in ascending fragment offset order.  When
	 *	the list is complete -- the fragment offset of the
	 *	first bundle is zero, the fragment offset of each
	 *	subsequent bundle is less than or equal to the sum
	 *	of the fragment offset and payload length of the
	 *	prior bundle, and the sum of the fragment offset and
	 *	payload length of the last bundle is equal to the
	 *	total ADU length -- the payloads are concatenated
	 *	into the original ADU, the payload for a new 
	 *	aggregate bundle that is queued for delivery to
	 *	the application, and all fragments (including the
	 *	final one) are destroyed.				*/

	unsigned int	totalAduLength;
} IncompleteBundle;

/*	*	*	Endpoint structures	*	*	*	*/

typedef enum
{
	DiscardBundle = 0,
	EnqueueBundle
} BpRecvRule;

/*	The endpoint object characterizes an endpoint within which
 *	the node is registered.						*/

typedef struct
{
	char		nss[MAX_NSS_LEN + 1];
	BpRecvRule	recvRule;
	Object		recvScript;	/*	SDR string		*/
	Object		incompletes;	/*	SDR list of Incompletes	*/
	Object		deliveryQueue;	/*	SDR list of Bundles	*/
	Object		scheme;		/*	back-reference		*/
	Object		stats;		/*	EndpointStats address.	*/
	int		updateStats;	/*	Boolean.		*/
} Endpoint;

#define	BP_ENDPOINT_SOURCED		0
#define	BP_ENDPOINT_QUEUED		1
#define	BP_ENDPOINT_ABANDONED		2
#define	BP_ENDPOINT_DELIVERED		3
#define BP_ENDPOINT_STATS		4

typedef struct
{
	time_t		resetTime;
	Tally		tallies[BP_ENDPOINT_STATS];
} EndpointStats;

/*	The volatile endpoint object encapsulates the volatile state
 *	of the corresponding Endpoint.					*/

typedef struct
{
	Object		endpointElt;	/*	Reference to Endpoint.	*/
	Object		stats;		/*	EndpointStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	char		nss[MAX_NSS_LEN + 1];
	int		appPid;		/*	Consumes dlv notices.	*/
	sm_SemId	semaphore;	/*	For dlv notices.	*/
} VEndpoint;

/*	*	*	Scheme structures	*	*	*	*/

/*	Scheme objects are used to encapsulate knowledge about how to
 *	forward bundles.  CBHE-conformant schemes are so noted.		*/

typedef struct
{
	char		name[MAX_SCHEME_NAME_LEN + 1];
	int		nameLength;
	int		cbhe;		/*	Boolean.		*/
	int		unicast;	/*	Boolean;		*/
	Object		fwdCmd; 	/*	For starting forwarder.	*/
	Object		admAppCmd; 	/*	For starting admin app.	*/
	Object		forwardQueue;	/*	SDR list of Bundles	*/
	Object		endpoints;	/*	SDR list of Endpoints	*/
} Scheme;

typedef struct
{
	Object		schemeElt;	/*	Reference to scheme.	*/

	/*	Copied from Scheme.	*/

	char		name[MAX_SCHEME_NAME_LEN + 1];
	int		nameLength;
	int		cbhe;
	int		unicast;

	char		adminEid[MAX_EID_LEN];
	int		adminNSSLength;
	int		fwdPid;		/*	For stopping forwarder.	*/
	int		admAppPid;	/*	For stopping admin app.	*/
	sm_SemId	semaphore;	/*	For dispatch notices.	*/
	PsmAddress	endpoints;	/*	SM list: VEndpoint.	*/
} VScheme;

/*	*	*	Induct structures	*	*	*	*/

typedef struct
{
	char		name[MAX_CL_DUCT_NAME_LEN + 1];
	Object		cliCmd;		/*	For starting the CLI.	*/
	Object		protocol;	/*	back-reference		*/
	Object		stats;		/*	InductStats address.	*/
	int		updateStats;	/*	Boolean.		*/
} Induct;

#define	BP_INDUCT_RECEIVED		0
#define	BP_INDUCT_MALFORMED		1
#define	BP_INDUCT_INAUTHENTIC		2
#define	BP_INDUCT_CONGESTIVE		3
#define	BP_INDUCT_STATS			4

typedef struct
{
	time_t		resetTime;
	Tally		tallies[BP_INDUCT_STATS];
} InductStats;

typedef struct
{
	Object		inductElt;	/*	Reference to Induct.	*/
	Object		stats;		/*	InductStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		cliPid;		/*	For stopping the CLI.	*/
	Throttle	acqThrottle;	/*	For congestion control.	*/
} VInduct;

/*	*	*	Outduct structures	*	*	*	*/

typedef struct
{
	Scalar		backlog;
	Object		lastForOrdinal;	/*	SDR list element.	*/
} OrdinalState;

typedef struct
{
	char		name[MAX_CL_DUCT_NAME_LEN + 1];
	Object		cloCmd;		/*	For starting the CLO.	*/
	Object		bulkQueue;	/*	SDR list of Bundles	*/
	Scalar		bulkBacklog;	/*	Bulk bytes enqueued.	*/
	Object		stdQueue;	/*	SDR list of Bundles	*/
	Scalar		stdBacklog;	/*	Std bytes enqueued.	*/
	Object		urgentQueue;	/*	SDR list of Bundles	*/
	Scalar		urgentBacklog;	/*	Urgent bytes enqueued.	*/
	OrdinalState	ordinals[256];	/*	Orders urgent queue.	*/
	unsigned int	maxPayloadLen;	/*	0 = no limit.		*/
	int		blocked;	/*	Boolean			*/
	Object		protocol;	/*	back-reference		*/
	Object		stats;		/*	OutductStats address.	*/
	int		updateStats;	/*	Boolean.		*/
} Outduct;

#define	BP_OUTDUCT_ENQUEUED		0
#define	BP_OUTDUCT_DEQUEUED		1
#define	BP_OUTDUCT_STATS		2

typedef struct
{
	time_t		resetTime;
	Tally		tallies[BP_OUTDUCT_STATS];
} OutductStats;

typedef struct
{
	Object		outductElt;	/*	Reference to Outduct.	*/
	Object		stats;		/*	OutductStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		cloPid;		/*	For stopping the CLO.	*/
	sm_SemId	semaphore;	/*	For transmit notices.	*/
	Throttle	xmitThrottle;	/*	For rate control.	*/
} VOutduct;

/*	*	*	Protocol structures	*	*	*	*/

typedef struct
{
	char		name[MAX_CL_PROTOCOL_NAME_LEN + 1];
	int		payloadBytesPerFrame;
	int		overheadPerFrame;
	int		nominalRate;	/*	Bytes per second.	*/
	Object		inducts;	/*	SDR list of Inducts	*/
	Object		outducts;	/*	SDR list of Outducts	*/
} ClProtocol;

/*	*	*	BP Database structures	*	*	*	*/

#define	EPOCH_2000_SEC	946684800

typedef enum
{
	expiredTTL = 1,
	xmitOverdue = 2,
	ctDue = 3,
	csDue = 4
} BpEventType;

typedef struct
{
	BpEventType	type;
	time_t		time;		/*	as from time(2)		*/
	Object		ref;		/*	Bundle, etc.		*/
} BpEvent;

typedef struct
{
	Object		bundleObj;	/*	0 if count > 1.		*/
	unsigned int	count;
} BundleSet;

typedef struct
{
	Object		schemes;	/*	SDR list of Schemes	*/
	Object		protocols;	/*	SDR list of ClProtocols	*/
	Object		timeline;	/*	SDR list of BpEvents	*/
	Object		bundles;	/*	SDR hash of BundleSets	*/
	Object		inboundBundles;	/*	SDR list of ZCOs	*/
	Object		limboQueue;	/*	SDR list of Bundles	*/
	Object		clockCmd; 	/*	For starting clock.	*/
	int		maxAcqInHeap;
	unsigned int	bundleCounter;	/*	For non-synced clock.	*/
	int		watching;	/*	Activity watch switch.	*/
	time_t		resetTime;	/*	Stats reset time.	*/
	Object		sourceStats;	/*	BpCosStats address.	*/
	Object		recvStats;	/*	BpCosStats address.	*/
	Object		discardStats;	/*	BpCosStats address.	*/
	Object		xmitStats;	/*	BpCosStats address.	*/
	Object		rptStats;	/*	BpRptStats address.	*/
	Object		ctStats;	/*	BpCtStats address.	*/
	Object		dbStats;	/*	BpDbStats address.	*/
	int		updateStats;	/*	Boolean.		*/
} BpDB;

/*  CT database encapsulates custody transfer configuration. */

typedef struct
{
	unsigned int	ctExpiredTimeout;
} BpctDB;

#define BP_STATUS_RECEIVE	0
#define BP_STATUS_ACCEPT	1
#define BP_STATUS_FORWARD	2
#define BP_STATUS_DELIVER	3
#define BP_STATUS_DELETE	4
#define BP_STATUS_STATS		5

#define BP_REASON_NONE		0
#define BP_REASON_EXPIRED	1
#define BP_REASON_FWD_UNIDIR	2
#define BP_REASON_CANCELED	3
#define BP_REASON_DEPLETION	4
#define BP_REASON_EID_MALFORMED	5
#define BP_REASON_NO_ROUTE	6
#define BP_REASON_NO_CONTACT	7
#define BP_REASON_BLK_MALFORMED	8
#define BP_REASON_STATS		9

#define	BP_CT_CUSTODY_ACCEPTED	0
#define	BP_CT_CUSTODY_RELEASED	1
#define	BP_CT_CUSTODY_EXPIRED	2
#define BP_CT_REDUNDANT		3
#define BP_CT_DEPLETION		4
#define BP_CT_EID_MALFORMED	5
#define BP_CT_NO_ROUTE		6
#define BP_CT_NO_CONTACT	7
#define BP_CT_BLK_MALFORMED	8
#define BP_CT_STATS		9

#define	BP_DB_QUEUED_FOR_FWD	0
#define	BP_DB_FWD_OKAY		1
#define	BP_DB_FWD_FAILED	2
#define	BP_DB_REQUEUED_FOR_FWD	3
#define	BP_DB_TO_LIMBO		4
#define	BP_DB_FROM_LIMBO	5
#define	BP_DB_EXPIRED		6
#define	BP_DB_STATS		7

typedef struct
{
	Tally		tallies[3];
} BpCosStats;

typedef struct
{
	unsigned int	totalRptByStatus[BP_STATUS_STATS];
	unsigned int	currentRptByStatus[BP_STATUS_STATS];
	unsigned int	totalRptByReason[BP_REASON_STATS];
	unsigned int	currentRptByReason[BP_REASON_STATS];
} BpRptStats;

typedef struct
{
	Tally		tallies[BP_CT_STATS];
} BpCtStats;

typedef struct
{
	Tally		tallies[BP_DB_STATS];
} BpDbStats;

/*	Volatile database encapsulates the volatile state of the
 *	database.							*/

typedef struct
{
	Object		sourceStats;	/*	BpCosStats address.	*/
	Object		recvStats;	/*	BpCosStats address.	*/
	Object		discardStats;	/*	BpCosStats address.	*/
	Object		xmitStats;	/*	BpCosStats address.	*/
	Object		rptStats;	/*	BpRptStats address.	*/
	Object		ctStats;	/*	BpCtStats address.	*/
	Object		dbStats;	/*	BpDbStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	unsigned int	creationTimeSec;
	int		bundleCounter;
	int		clockPid;	/*	For stopping clock.	*/
	int		watching;	/*	Activity watch switch.	*/

	/*	For finding structures in database.			*/

	PsmAddress	schemes;	/*	SM list: VScheme.	*/
	PsmAddress	inducts;	/*	SM list: VInduct.	*/
	PsmAddress	outducts;	/*	SM list: VOutduct.	*/
	PsmAddress	timeline;	/*	SM RB tree: list xref.	*/
} BpVdb;

/*	*	*	Acquisition structures	*	*	*	*/

typedef enum
{
	AcqTBD = 0,
	AcqOK
} AcqDecision;

typedef struct
{
	VInduct		*vduct;

	/*	Per-bundle state variables.				*/

	Bundle		bundle;
	int		headerLength;
	int		trailerLength;
	int		bundleLength;
	int		authentic;	/*	Boolean.		*/
	char		*dictionary;
	Lyst		collabBlocks;	
	Lyst		extBlocks[2];	/*	(AcqExtBlock *)		*/
	int		currentExtBlocksList;	/*	0 or 1.		*/
	AcqDecision	decision;
	int		lastBlockParsed;
	int		malformed;
	int		congestive;	/*	Not enough ZCO space.	*/
	int		mustAbort;	/*	Unreadable block(s).	*/

	/*	Per-acquisition state variables.			*/

	int		allAuthentic;	/*	Boolean.		*/
	char		*senderEid;
	Object		acqFileRef;
	Object		zco;		/*	Concatenated bundles.	*/
	Object		zcoElt;		/*	Retention in BpDB.	*/
	int		zcoLength;
	int		zcoBytesConsumed;
	ZcoReader	reader;
	int		zcoBytesReceived;
	int		bytesBuffered;
	char		buffer[BP_MAX_BLOCK_SIZE];
} AcqWorkArea;

/*	Definitions supporting route computation.			*/

typedef enum
{
	fwd = 1,	/*	Forward via indicated EID.		*/
	xmit = 2	/*	Transmit in indicated CL duct.		*/
} FwdAction;

typedef struct
{
	FwdAction	action;
	Object		outductElt;	/*	sdrlist elt for xmit	*/
	Object		destDuctName;	/*	sdrstring for xmit	*/
	Object		eid;		/*	sdrstring for fwd	*/
} FwdDirective;

/*	Definitions supporting determination of sender endpoint ID.	*/

typedef int		(*BpEidLookupFn)(char *uriBuffer, char *neighborClId);

extern BpEidLookupFn	*senderEidLookupFunctions(BpEidLookupFn fn);
extern void		getSenderEid(char **eidBuffer, char *neighborClId);
extern int		clIdMatches(char *neighborClId, FwdDirective *dir);

extern void		getSenderEid(char **eidBuffer, char *neighborClId);

/*	Definitions supporting the use of QoS-sensitive bandwidth
 *	management.							*/

/*	Outflow objects are non-persistent.  They are used only to
 *	do QoS-sensitive bandwidth management.  Since they are private
 *	to CLOs, they can reside in the private memory of the CLO
 *	(rather than in shared memory).
 *
 *	Each Outduct, of whatever CL protocol, has three Outflows:
 *
 *		Expedited = 2, Standard = 1, and Bulk = 0.
 *
 * 	Expedited traffic goes out before any other.  Standard traffic
 * 	(service factor = 2) goes out twice as fast as Bulk traffic
 * 	(service factor = 1).                 				*/

#define EXPEDITED_FLOW   2

typedef struct
{
	Object		outboundBundles;	/*	SDR list.	*/
	int		totalBytesSent;
	int		svcFactor;
} Outflow;

/*	*	*	Function prototypes.	*	*	*	*/

extern int		bpSend(		MetaEid *sourceMetaEid,
					char *destEid,
					char *reportToEid,
					int lifespan,
					int classOfService,
					BpCustodySwitch custodySwitch,
					unsigned char srrFlags,
					int ackRequested,
					BpExtendedCOS *extendedCOS,
					Object adu,
					Object *newBundle,
					int bundleIsAdmin);
			/*	This function creates a new bundle
			 *	and queues it for forwarding by the
			 *	applicable scheme-specific forwarder,
			 *	based on the scheme name of the
			 *	destination endpoint ID.
			 *
			 *	adu must be a zero-copy object
			 *	reference as returned by zco_create().
			 *	bundleIsAdmin is Boolean, must be
			 *	1 for status reports and custody
			 *	signals but zero otherwise.
			 *
			 *	Returns 1 on success, in which case
			 *	(and only in this case) the address
			 *	of the new bundle within the ION
			 *	database is returned in newBundle.
			 *	If destination is found to be NULL
			 *	(a transient error), returns zero.
			 *	Otherwise (permanent system failure)
			 *	returns -1.				*/

extern int		bpAbandon(	Object bundleObj,
					Bundle *bundle);
			/*	This is the common processing for any
			 *	bundle that a forwarder decides it
			 *	cannot accept for forwarding.  It 
			 *	sends any applicable status reports
			 *	and then deletes the bundle from
			 *	local storage.
			 *
			 *	Call this function at most once per
			 *	bundle.	 Returns 0 on success, -1 on
			 *	any failure.				*/

extern int		bpAccept(	Object bundleObj,
					Bundle *bundle);
			/*	This is the common processing for any
			 *	bundle that a forwarder decides it
			 *	can accept for forwarding, whether
			 *	the bundle was sourced locally or
			 *	was received from some other node.
			 *	It updates statistics that are used
			 *	to make future bundle acquisition
			 *	decisions; if custody transfer is
			 *	requested, it takes custody of the
			 *	bundle; and it sends any applicable
			 *	status reports.
			 *
			 *	This function may be called multiple
			 *	times per bundle but will take effect
			 *	only once.  Returns 0 on success, -1
			 *	on any failure.				*/

extern int		bpClone(	Bundle *originalBundle,
					Bundle *newBundleBuffer,
					Object *newBundleObj,
					unsigned int offset,
					unsigned int length);
			/*	The function creates a copy of part
			 *	or all of originalBundle, writing
			 *	it to the location returned in
			 *	newBundleObj.  newBundleBuffer is
			 *	required: it is used as the work
			 *	area for constructing the new bundle.
			 *	If offset is zero and length is the
			 *	length of originalBundle's payload
			 *	(or zero, which is used to indicate
			 *	"length of original bundle's payload")
			 *	then the copy will have a copy of
			 *	the entire payload of the original
			 *	bundle.  Otherwise the function is
			 *	used to "fragment" the bundle: the
			 *	new bundle's payload will be the
			 *	indicated subset of the original
			 *	payload.  Returns 0 on success,
			 *	-1 on any error.			*/

extern int		bpEnqueue(	FwdDirective *directive,
					Bundle *bundle,
					Object bundleObj,
					char *proxNodeEid);
			/*	This function is invoked by a forwarder
			 *	to enqueue a bundle for transmission
			 *	by a convergence-layer output adapter
			 *	to the proximate node identified by
			 *	proxNodeEid.  It appends the indicated
			 *	bundle to the appropriate transmission
			 *	queue of the duct indicated by
			 *	"directive" based on the bundle's
			 *	priority.  If the bundle is destined
			 *	for a specific location among all the
			 *	locations that are reachable via this
			 *	duct, then the directive' destDuctName
			 *	must be a string identifying that
			 *	location.
			 *
			 *	If forwarding the bundle to multiple
			 *	nodes (flooding or multicast), call
			 *	this function once per planned
			 *	transmission.
			 *
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern int		bpDequeue(	VOutduct *vduct,
					Outflow *outflows,
					Object *outboundZco,
					BpExtendedCOS *extendedCOS,
					char *destDuctName,
					unsigned int maxPayloadLength,
					int timeoutInterval);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) to get a bundle that it is to
			 *	transmit to some remote convergence-
			 *	layer input adapter (induct).
			 *
			 *	bpDequeue first blocks until the
			 *	capacity of the outduct's xmitThrottle
			 *	is non-negative.  In this way BP imposes
			 *	rate control on outbound traffic,
			 *	limiting transmission rate to the
			 *	nominal data rate of the outduct.
			 *
			 *	The function then selects the next
			 *	outbound bundle from the set of outduct
			 *	bundle queues identified by outflows.
			 *	If no such bundle is currently waiting
			 *	for transmission, it blocks until one
			 *	is [or until a signal handler calls
			 *	bp_interrupt()].
			 *
			 *	On selecting a bundle, if the bundle's
			 *	payload is longer than the indicated
			 *	maximum then bpDequeue fragments the
			 *	bundle: a cloned bundle whose payload
			 *	is all bytes beyond the initial
			 *	maxPayloadLength bytes is created and
			 *	inserted back to the front of the queue
			 *	from which the bundle was selected.
			 *
			 *	Then bpDequeue catenates (serializes)
			 *	the BP header information in the
			 *	bundle and prepends that serialized
			 *	header to the source data of the
			 *	bundle's payload ZCO; if there are
			 *	post-payload blocks, it likewise
			 *	catenates them into a trailer that
			 *	is appended to the source data.  Then
			 *	it returns the address of that ZCO in
			 *	*bundleZco for transmission at the
			 *	convergence layer (possibly entailing
			 *	segmentation that would be invisible
			 *	to BP).
			 *
			 *	The extended class of service for the
			 *	bundle is provided in *extendedCOS
			 *	so that the requested QOS can be
			 *	mapped to the QOS features of the
			 *	convergence-layer protocol.
			 *
			 *	If this bundle is destined for a
			 *	specific location among all the
			 *	locations that are reachable via
			 *	this duct, then a string identifying
			 *	that location is written into
			 *	destDuctName, which must be an array
			 *	of at least MAX_CL_DUCT_NAME_LEN + 1
			 *	bytes.
			 *
			 *	The timeoutInterval argument indicates
			 *	the length of time following return of
			 *	a bundle after which failure of custody
			 *	transfer should be inferred if no
			 *	custody acceptance signal for this
			 *	bundle has been received.  Any negative
			 *	value signifies that the calling
			 *	function accepts "stewardship" of the
			 *	bundle, i.e., commits to dispositioning
			 *	the returned bundle as soon as results
			 *	of convergence-layer transmission are
			 *	known, by calling one of two functions:
			 *	either bpHandleXmitSuccess or else
			 *	bpHandleXmitFailure.  A value of zero
			 *	signifies that the calling function
			 *	does not accept stewardship but also
			 *	does not want to set a custody transfer
			 *	timer.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpIdentify(Object bundleZco, Object *bundleObj);
			/*	This function parses out the ID fields
			 *	of the catenated outbound bundle in
			 *	bundleZco, locates the bundle that
			 *	is identified by that bundle ID, and
			 *	passes that bundle's address back in
			 *	bundleObj, e.g., to enable insertion
			 *	of a custody timeout event via bpMemo.
			 *
			 *	bpIdentify allocates a temporary
			 *	buffer of size BP_MAX_BLOCK_SIZE
			 *	into which the initial block(s) of
			 *	the catenated bundle identified
			 *	by bundleZco are read for parsing.
			 *	If that buffer is not long enough
			 *	to contain the entire primary block
			 *	of the bundle, bundle ID field
			 *	extraction will be incomplete and
			 *	the bundle will not be located.
			 *
			 *	If the bundle is not located, the
			 *	address returned in *bundleObj will
			 *	be zero.
			 *
			 *	Returns 0 on success (whether bundle
			 *	was located or not), -1 on system
			 *	failure.				*/

extern int		bpMemo(Object bundleObj, unsigned int interval); 
			/*	This function inserts a "custody-
			 *	acceptance due" event into the
			 *	timeline.  The event causes bpclock
			 *	to re-forward the indicated bundle if
			 *	it is still in the database (i.e., it
			 *	has not yet been accepted by another
			 *	custodian) as of the moment computed
			 *	by adding the indicated interval to
			 *	the current time.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpHandleXmitSuccess(Object zco, unsigned int interval);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) on detection of convergence-
			 *	layer protocol transmission success.
			 *	It causes the serialized (catenated)
			 *	outbound bundle in zco to be destroyed,
			 *	unless some constraint (such as
			 *	acceptance of custody) requires that
			 *	bundle destruction be deferred.
			 *
			 *	In the event that custody of the
			 *	bundle has indeed been accepted, the
			 *	calling function can request a further
			 *	increment of reliability: when the
			 *	value of "interval" is greater than
			 *	zero, it indicates the length of time
			 *	after which failure of custody transfer
			 *	should be inferred if no custody
			 *	acceptance signal for this bundle has
			 *	been received.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpHandleXmitFailure(Object zco);
			/*	This function is invoked by a
			 *	convergence-layer output adapter (an
			 *	outduct) on detection of a convergence-
			 *	layer protocol transmission error.
			 *	It causes the serialized (catenated)
			 *	outbound bundle in zco to be queued
			 *	up for re-forwarding.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern int		bpReforwardBundle(Object bundleToReforward);
			/*	bpReforwardBundle aborts the current
			 *	outduct queuing for the bundle and
			 *	queues it for re-forwarding, possibly
			 *	on a different route.  It is invoked
			 *	when transmission is determined to be
			 *	overdue, indicating an anomaly in the
			 *	originally selected route.
			 *
			 *	Returns 0 on success, -1 on failure.	*/

extern AcqWorkArea	*bpGetAcqArea(VInduct *vduct);
			/*	Allocates a bundle acquisition work
			 *	area for use in acquiring inbound
			 *	bundles via the indicated duct.
			 *
			 *	Returns NULL on any failure.		*/

extern void		bpReleaseAcqArea(AcqWorkArea *workArea);
			/*	Releases dynamically allocated
			 *	bundle acquisition work area.		*/

extern int		bpBeginAcq(	AcqWorkArea *workArea,
					int authentic,
					char *senderEid);
			/*	This function is invoked by a
			 *	convergence-layer input adapter
			 *	to initiate acquisition of a new
			 *	bundle via the indicated workArea.
			 *	It initializes deserialization of
			 *	an array of bytes constituting a
			 *	single transmitted bundle.
			 *
			 *	The "authentic" Boolean and "senderEid"
			 *	string are knowledge asserted by the
			 *	convergence-layer input adapter
			 *	invoking this function: an assertion
			 *	of authenticity of the data being
			 *	acquired (e.g., per knowledge that
			 *	the data were received via a
			 *	physically secure medium) and, if
			 *	non-NULL, an EID characterizing the
			 *	node that send this inbound bundle.
			 *
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern int		bpLoadAcq(	AcqWorkArea *workArea,
					Object zco);
			/*	This function continues acquisition
			 *	of a bundle as initiated by an
			 *	invocation of bpBeginAcq().  To
			 *	do so, it inserts the indicated
			 *	zero-copy object -- containing
			 *	the bundle in concatenated form --
			 *	into workArea.
			 *
			 *	bpLoadAcq is an alternative to
			 *	bpContinueAcq, intended for use
			 *	by convergence-layer adapters that
			 *	natively acquire concatenated
			 *	bundles into zero-copy objects.
			 *
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern int		bpContinueAcq(	AcqWorkArea *workArea,
					char *bytes,
					int length);
			/*	This function continues acquisition
			 *	of a bundle as initiated by an
			 *	invocation of bpBeginAcq().  To
			 *	do so, it appends the indicated array
			 *	of bytes, of the indicated length, to
			 *	the byte array that is encapsulated
			 *	in workArea.
			 *
			 *	bpContinueAcq is an alternative to
			 *	bpLoadAcq, intended for use by
			 *	convergence-layer adapters that
			 *	incrementally acquire portions of
			 *	concatenated bundles into byte-array
			 *	buffers.  The function transparently
			 *	creates a zero-copy object for
			 *	acquisition of the bundle, if one
			 *	does not already exist, and appends
			 *	"bytes" to the source data of that
			 *	ZCO.
			 *	
			 *	Returns 0 on success, -1 on any
			 *	failure.				*/

extern void		bpCancelAcq(	AcqWorkArea *workArea);
			/*	Cancels acquisition of a new
			 *	bundle via the indicated workArea,
			 *	destroying the bundle acquisition
			 *	ZCO of workArea.			*/

extern int		bpEndAcq(	AcqWorkArea *workArea);
			/*	Concludes acquisition of a new
			 *	bundle via the indicated workArea.
			 *	This function is invoked after the
			 *	convergence-layer input adapter
			 *	has invoked either bpLoadAcq() or
			 *	bpContinueAcq() [perhaps invoking
			 *	the latter multiple times] such
			 *	that all bytes of the transmitted
			 *	bundle are now included in the
			 *	bundle acquisition ZCO of workArea.
			 *
			 *	Returns 1 on success, 0 on any failure
			 *	pertaining only to this bundle, -1 on
			 *	any other (i.e., system) failure.  If
			 *	1 is returned, then the bundle has been
			 *	fully acquired and dispatched (that is,
			 *	queued for delivery and/or forwarding).
			 *	In this case, the invoking convergence-
			 *	layer input adapter should simply
			 *	continue with the next cycle of
			 *	bundle acquisition, i.e., it should
			 *	call bpBeginAcq().
			 *
			 *	If 0 is returned then the failure
			 *	is transient, applying only to the
			 *	bundle that is currently being
			 *	acquired.  In this case, the current
			 *	bundle acquisition has failed but
			 *	BP itself can continue; the invoking
			 *	convergence-layer input adapter
			 *	should simply continue with the next
			 *	cycle of bundle acquisition just as
			 *	if the return code had been 1.		*/

extern int		bpDestroyBundle(Object bundleToDestroy,
					int expired);
			/*	bpDestroyBundle destroys the bundle,
			 *	provided all retention constraints
			 *	have been removed.  "expired" is
			 *	Boolean, set to 1 only by bpClock when
			 *	it destroys a bundle whose TTL has
			 *	expired or by bp_cancel on bundle
			 *	cancellation.  Returns 1 if bundlex
			 *	is actually destroyed, 0 if bundle is
			 *	retained because not all constraints
			 *	have been removed, -1 on any error.	*/

extern int		bpInit();
extern void		bpDropVdb();
extern void		bpRaiseVdb();
extern int		bpSetCTCountdownTimer(time_t newTimeout);
extern int		bpStart();
extern void		bpStop();
extern int		bpAttach();
extern void		bpDetach();

extern Object		getBpDbObject();
extern BpDB		*getBpConstants();
extern BpVdb		*getBpVdb();

extern void		getCurrentDtnTime(DtnTime *dt);

extern int		guessBundleSize(Bundle *bundle);
extern int		computeECCC(int bundleSize, ClProtocol *protocol);
extern void		computeApplicableBacklog(Outduct *, Bundle *, Scalar *);

extern int		putBpString(BpString *bpString, char *string);
extern char		*getBpString(BpString *bpString);

extern char		*retrieveDictionary(Bundle *bundle);
extern void		releaseDictionary(char *dictionary);

extern int		parseEidString(char *eidString, MetaEid *metaEid,
				VScheme **scheme, PsmAddress *schemeElt);
extern void		restoreEidString(MetaEid *metaEid);
extern int		printEid(EndpointId *eid, char *dictionary, char **str);

extern int		startBpTask(Object cmd, Object cmdparms, int *pid);

extern void		noteStateStats(int stateIdx, Bundle *bundle);
extern void		clearAllStateStats();
extern void		reportAllStateStats();

extern void		findScheme(char *name, VScheme **vscheme,
				PsmAddress *elt);
extern int		addScheme(char *name, char *fwdCmd, char *admAppCmd);
extern int		updateScheme(char *name, char *fwdCmd, char *admAppCmd);
extern int		removeScheme(char *name);
extern int		bpStartScheme(char *name);
extern void		bpStopScheme(char *name);

extern void		findEndpoint(char *schemeName, char *nss,
				VScheme *vscheme, VEndpoint **vpoint,
				PsmAddress *elt);
/*	Note that adding an endpoint is also called "registering".	*/
extern int		addEndpoint(char *endpointName,
				BpRecvRule recvAction, char *recvScript);
extern int		updateEndpoint(char *endpointName,
				BpRecvRule recvAction, char *recvScript);
/*	Removing an endpoint is also called "unregistering".		*/
extern int		removeEndpoint(char *endpointName);

extern void		fetchProtocol(char *name, ClProtocol *clp, Object *elt);
extern int		addProtocol(char *name, int payloadBytesPerFrame,
				int overheadPerFrame, int nominalRate);
extern int		removeProtocol(char *name);
extern int		bpStartProtocol(char *name);
extern void		bpStopProtocol(char *name);

extern void		findInduct(char *protocolName, char *name,
				VInduct **vduct, PsmAddress *elt);
extern int		addInduct(char *protocolName, char *name,
				char *cliCmd);
extern int		updateInduct(char *protocolName, char *name,
				char *cliCmd);
extern int		removeInduct(char *protocolName, char *name);
extern int		bpStartInduct(char *protocolName, char *ductName);
extern void		bpStopInduct(char *protocolName, char *ductName);

extern void		findOutduct(char *protocolName, char *name,
				VOutduct **vduct, PsmAddress *elt);
extern int		addOutduct(char *protocolName, char *name,
				char *cloCmd, unsigned int maxPayloadLength);
extern int		updateOutduct(char *protocolName, char *name,
				char *cloCmd, unsigned int maxPayloadLength);
extern int		removeOutduct(char *protocolName, char *name);
extern int		bpStartOutduct(char *protocolName, char *ductName);
extern void		bpStopOutduct(char *protocolName, char *ductName);
extern int		bpBlockOutduct(char *protocolName, char *ductName);
extern int		bpUnblockOutduct(char *protocolName, char *ductName);

extern Object		insertBpTimelineEvent(BpEvent *newEvent);
extern void		destroyBpTimelineEvent(Object timelineElt);

extern int		findBundle(char *sourceEid, BpTimestamp *creationTime,
				unsigned int fragmentOffset,
				unsigned int fragmentLength,
				Object *bundleAddr);
extern int		retrieveInTransitBundle(Object bundleZco, Object *obj);

extern int		forwardBundle(Object bundleObj, Bundle *bundle,
				char *stationEid);

extern int		reverseEnqueue(Object xmitElt, ClProtocol *protocol,
				Object outductObj, Outduct *outduct,
				int sendToLimbo);

extern int		enqueueToLimbo(Bundle *bundle, Object bundleObj);
extern int		releaseFromLimbo(Object xmitElt, int resume);

extern int		sendStatusRpt(Bundle *bundle, char *dictionary);

typedef int		(*StatusRptCB)(BpDelivery *, BpStatusRpt *);
typedef int		(*CtSignalCB)(BpDelivery *, BpCtSignal *);

typedef struct bpsap_st
{
	VEndpoint	*vpoint;
	MetaEid		endpointMetaEid;
	sm_SemId	recvSemaphore;
} Sap;

extern int		_handleAdminBundles(char *adminEid,
				StatusRptCB handleStatusRpt,
				CtSignalCB handleCtSignal);
extern int		applyCtSignal(BpCtSignal *, char *);
extern int		eidIsLocal(EndpointId eid, char* dictionary);

#ifdef __cplusplus
}
#endif

#endif  /* _BPP_H_ */
