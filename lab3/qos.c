#include "rte_common.h"
#include "rte_cycles.h"
#include "rte_meter.h"
#include "rte_red.h"

#include "qos.h"

// We need fraction
const double NANO_PERSEC = 1000000000.0;

static struct rte_meter_srtcm_params app_srtcm_params[APP_FLOWS_MAX] = {
    {.cir = 160000000, .cbs = 10000 * 40, .ebs = 10000 * 48}, // Flow0
    {.cir = 80000000, .cbs = 10000 * 7, .ebs = 10000 * 36},   // Flow1
    {.cir = 40000000, .cbs = 1000 * 32, .ebs = 10000 * 20},   // Flow2
    {.cir = 20000000, .cbs = 1000 * 16, .ebs = 10000 * 16}    // Flow3
};

static struct rte_meter_srtcm app_flows[APP_FLOWS_MAX];

// The time passed to rte_meter should be counted from rte_meter config
static uint64_t start_times[APP_FLOWS_MAX];

static uint64_t cpu_hz;

/**
 * This function will be called only once at the beginning of the test. 
 * You can initialize your meter here.
 * 
 * int rte_meter_srtcm_config(struct rte_meter_srtcm *m, struct rte_meter_srtcm_params *params);
 * @return: 0 upon success, error code otherwise
 * 
 * void rte_exit(int exit_code, const char *format, ...)
 * #define rte_panic(...) rte_panic_(__func__, __VA_ARGS__, "dummy")
 * 
 * uint64_t rte_get_tsc_hz(void)
 * @return: The frequency of the RDTSC timer resolution
 * 
 * static inline uint64_t rte_get_tsc_cycles(void)
 * @return: The time base for this lcore.
 */
int qos_meter_init(void)
{
    uint32_t i;
    int ret;

    // Log the hz of current cpu
    cpu_hz = rte_get_tsc_hz();
    printf("QoS Meter: hz = %lu\n", cpu_hz);

    for (i = 0; i < APP_FLOWS_MAX; i++)
    {
        start_times[i] = rte_get_tsc_cycles();
        ret = rte_meter_srtcm_config(&app_flows[i], &app_srtcm_params[i]);
        if (ret < 0)
            return ret;
    }

    return 0;
}

/**
 * This function will be called for every packet in the test, 
 * after which the packet is marked by returning the corresponding color.
 * 
 * A packet is marked green if it doesn't exceed the CBS, 
 * yellow if it does exceed the CBS, but not the EBS, and red otherwise
 * 
 * The pkt_len is in bytes, the time is in nanoseconds.
 * 
 * Point: We need to convert ns to cpu circles
 * Point: Time is not counted from 0
 * 
 * static inline enum rte_meter_color rte_meter_srtcm_color_blind_check(struct rte_meter_srtcm *m,
	uint64_t time, uint32_t pkt_len)
 * 
 * enum qos_color { GREEN = 0, YELLOW, RED };
 * enum rte_meter_color { e_RTE_METER_GREEN = 0, e_RTE_METER_YELLOW,  
	e_RTE_METER_RED, e_RTE_METER_COLORS };
 */
enum qos_color
qos_meter_run(uint32_t flow_id, uint32_t pkt_len, uint64_t time)
{
    uint64_t ctime = start_times[flow_id] + (uint64_t)(time / NANO_PERSEC * cpu_hz);
    return (enum qos_color)rte_meter_srtcm_color_blind_check(&app_flows[flow_id], ctime, pkt_len);
}

static struct rte_red_params app_red_params[APP_FLOWS_MAX][e_RTE_METER_COLORS] = {
    {
        // Flow 0
        {.min_th = 1022, .max_th = 1023, .maxp_inv = 255, .wq_log2 = 9}, // Green
        {.min_th = 16, .max_th = 32, .maxp_inv = 5, .wq_log2 = 9},       // Yellow
        {.min_th = 1, .max_th = 16, .maxp_inv = 1, .wq_log2 = 9}         // Red
    },
    {
        // Flow 1
        {.min_th = 64, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9}, // Green
        {.min_th = 16, .max_th = 32, .maxp_inv = 3, .wq_log2 = 9},    // Yellow
        {.min_th = 1, .max_th = 16, .maxp_inv = 1, .wq_log2 = 9}      // Red
    },
    {
        // Flow 2
        {.min_th = 64, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9}, // Green
        {.min_th = 4, .max_th = 16, .maxp_inv = 2, .wq_log2 = 9},     // Yellow
        {.min_th = 1, .max_th = 8, .maxp_inv = 1, .wq_log2 = 9}       // Red
    },
    {
        // Flow 3
        {.min_th = 64, .max_th = 1023, .maxp_inv = 10, .wq_log2 = 9}, // Green
        {.min_th = 2, .max_th = 6, .maxp_inv = 3, .wq_log2 = 9},      // Yellow
        {.min_th = 1, .max_th = 3, .maxp_inv = 1, .wq_log2 = 9}       // Red
    }};

static struct rte_red_config app_red_configs[APP_FLOWS_MAX][e_RTE_METER_COLORS];

// Run-time data
static struct rte_red app_reds[APP_FLOWS_MAX][e_RTE_METER_COLORS];

// Queue size in packets
unsigned queues[APP_FLOWS_MAX];

// Last time of burst
uint64_t last_burst_time;

/**
 * This function will be called only once at the beginning of the test. 
 * You can initialize you dropper here
 * 
 * int rte_red_rt_data_init(struct rte_red *red);
 * @return Operation status, 0 success
 * 
 * int rte_red_config_init(struct rte_red_config *red_cfg, const uint16_t wq_log2, 
   const uint16_t min_th, const uint16_t max_th, const uint16_t maxp_inv);
 * @return Operation status, 0 success 
 */
int qos_dropper_init(void)
{
    uint32_t i, j;
    int ret;

    for (i = 0; i < APP_FLOWS_MAX; i++)
    {
        for (j = 0; j < e_RTE_METER_COLORS; j++)
        {
            // Initialize red run-time data
            ret = rte_red_rt_data_init(&app_reds[i][j]);
            if (ret < 0)
                return ret;

            // Configure RED configuration
            ret = rte_red_config_init(&app_red_configs[i][j], app_red_params[i][j].wq_log2, app_red_params[i][j].min_th, app_red_params[i][j].max_th, app_red_params[i][j].maxp_inv);
            if (ret < 0)
                return ret;
        }

        // Initialize queue
        queues[i] = 0;
    }

    last_burst_time = 0;

    return 0;
}

/**
 * This function will be called for every tested packet after being marked by the meter, 
 * and will make the decision whether to drop the packet by returning the decision (0 pass, 1 drop)
 * 
 * The probability of drop increases as the estimated average queue size grows
 * 
 * static inline void rte_red_mark_queue_empty(struct rte_red *red, const uint64_t time)
 * @brief Callback to records time that queue became empty
 * @param q_time : Start of the queue idle time (q_time) 
 * 
 * static inline int rte_red_enqueue(const struct rte_red_config *red_cfg,
	struct rte_red *red, const unsigned q, const uint64_t time)
 * @param q [in] updated queue size in packets   
 * @return Operation status
 * @retval 0 enqueue the packet
 * @retval 1 drop the packet based on max threshold criteria
 * @retval 2 drop the packet based on mark probability criteria
 */
int qos_dropper_run(uint32_t flow_id, enum qos_color color, uint64_t time)
{

    // Send out all packets in queues
    if (time != last_burst_time)
    {
        for (int i = 0; i < APP_FLOWS_MAX; i++)
            queues[i] = 0;
        last_burst_time = time;
    }

    // Actually the timestamp should be measured in bytes, however we could not get the timestamp in bytes in such an interface.
    // So just pass cpu circles as timestamp

    uint64_t ctime = start_times[flow_id] + (uint64_t)(time / NANO_PERSEC * cpu_hz);
    if (rte_red_enqueue(&app_red_configs[flow_id][color], &app_reds[flow_id][color], queues[flow_id], ctime) == 0)
    {
        queues[flow_id]++;
        return 0;
    }

    return 1;
}
