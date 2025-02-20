/***
 * Simple CLI program to test the sharemap interface.
 */
#include "sharemap.hpp"
#include "udp.hpp"
#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

// Sharemap proxy settings
std::string sharemap_control_url = "udp://0.0.0.0:3333";
std::string sharemap_metrics_url = "udp://127.0.0.1:4444";

anysignal::udp_sock *control_socket = nullptr;
anysignal::udp_sock *metrics_socket = nullptr;
anysignal::sharemap_config_t config;
anysignal::sharemap_metrics_t metrics;

std::thread recv_thread;

static volatile bool metrics_initialized = false;

// Signal handler
static volatile bool running = true;
static volatile bool receiving = false;
static void signal_callback_handler(int signum)
{
    printf("Caught signal %d\n", signum);
    receiving = false;
    running = false;
    // stop calls blocking on getline stdin
    fclose(stdin);
}

void recv_metrics()
{
    anysignal::sharemap_metrics_t::packed_t packed_metrics;
    while (receiving)
    {

        // Wait for metrics
        if (metrics_socket->recv_ready(std::chrono::milliseconds(500)))
        {
            // Receive packed data
            size_t recvd = metrics_socket->recv(reinterpret_cast<uint8_t *>(&packed_metrics),
                                                anysignal::sharemap_metrics_t::PACKED_SIZE);
            if (recvd != anysignal::sharemap_metrics_t::PACKED_SIZE)
            {
                continue;
            }

            // Unpack
            metrics = anysignal::sharemap_unpack(packed_metrics);

            // Check the hash
            if (metrics.schema_hash != anysignal::sharemap_metrics_t::HASH)
            {
                printf("Unexpected schema hash (0x%lX)\n", metrics.schema_hash);
            }

            metrics_initialized = true;
        }
    }
}

void help()
{
    std::cout << "Usage:  <command> [<args>]" << std::endl;
    std::cout << "    Supported commands:" << std::endl;
    std::cout << "    help                Display this help" << std::endl;
    std::cout << "    set <key> <value>   Set a value" << std::endl;
    std::cout << "    send config         Send the configuration" << std::endl;
    std::cout << "    connect             Connect to sharemap server" << std::endl;
    std::cout << "    disconnect          Disconnect from sharemap server" << std::endl;
    std::cout << "    display <what>      Display info.  <what> can be \"config\" or \"metrics\"" << std::endl;
    std::cout << "    quit                Quit this application" << std::endl;
}

// Helper functions for conversion to and from strings
static inline void set(std::string& lhs, std::string& rhs)
{
    if (rhs == "\"\"" || rhs == "''" || rhs == "{}")
        lhs = "";
    else
        lhs = rhs;
}

static inline void set(bool& lhs, std::string& rhs)
{
    lhs = (rhs == "true" or rhs == "1" ? true : false);
}

static inline void set(std::uint8_t& lhs, std::string& rhs)
{
    lhs = uint8_t(stoul(rhs));
}

static inline void set(std::uint16_t& lhs, std::string& rhs)
{
    lhs = uint16_t(stoul(rhs));
}

static inline void set(std::uint32_t& lhs, std::string& rhs)
{
    lhs = uint32_t(stoul(rhs));
}

static inline void set(std::uint64_t& lhs, std::string& rhs)
{
    lhs = stoull(rhs);
}

static inline void set(std::int8_t& lhs, std::string& rhs)
{
    lhs = int8_t(stoi(rhs));
}

static inline void set(std::int16_t& lhs, std::string& rhs)
{
    lhs = int16_t(stoi(rhs));
}

static inline void set(std::int32_t& lhs, std::string& rhs)
{
    lhs = stol(rhs);
}

static inline void set(std::int64_t& lhs, std::string& rhs)
{
    lhs = stoll(rhs);
}

static inline void set(double& lhs, std::string& rhs)
{
    lhs = stod(rhs);
}

static inline void set(float& lhs, std::string& rhs)
{
    lhs = stof(rhs);
}

static inline void set(std::array<char, anysignal::STRING_BUFFER_SIZE>& lhs, std::string& rhs)
{
    std::memset(reinterpret_cast<char *>(lhs.data()), 0, lhs.max_size());
    if (rhs != "\"\"" && rhs != "''" && rhs != "{}")
    {
        std::memcpy(reinterpret_cast<char *>(lhs.data()),
                    reinterpret_cast<char *>(rhs.data()),
                    std::min(lhs.max_size() - 1, rhs.length()));
    }
}

static inline std::string to_string(std::string& val)
{
    return val;
}

static inline std::string to_string(bool& val)
{
    return val ? std::string("true") : std::string("false");
}

static inline std::string to_string(uint8_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(uint16_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(uint32_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(uint64_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(int8_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(int16_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(int32_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(int64_t& val)
{
    return std::to_string(val);
}

static inline std::string to_string(float& val)
{
    return std::to_string(val);
}

static inline std::string to_string(double& val)
{
    return std::to_string(val);
}

static inline std::string to_string(std::array<char, anysignal::STRING_BUFFER_SIZE>& val)
{
    return std::string(val.begin(), val.end());
}

void set(std::string args)
{
    auto pos = args.find_first_of(" ");
    auto key = args.substr(0, pos);
    auto val = pos == std::string::npos ? std::string("") : args.substr(pos + 1);

    if (key.empty())
    {
        std::cout << "Missing <key>" << std::endl;
        return;
    }
    else if (val.empty())
    {
        std::cout << "Missing <value>" << std::endl;
        return;
    }
    else if (key == "sharemap_control_url")
    {
        sharemap_control_url = val;
    }
    else if (key == "sharemap_metrics_url")
    {
        sharemap_metrics_url = val;
    }
    else if (key == "source_id")
    {
        set(config.source_id, val);
    }
    else if (key == "schema_hash")
    {
        set(config.schema_hash, val);
    }
    else if (key == "unix_timestamp_ns")
    {
        set(config.unix_timestamp_ns, val);
    }
    else if (key == "psk_cc_tx_force_on")
    {
        set(config.psk_cc_tx_force_on, val);
    }
    else if (key == "psk_cc_tx_idle_timeout_s")
    {
        set(config.psk_cc_tx_idle_timeout_s, val);
    }
    else if (key == "psk_cc_tx_fe_frequency")
    {
        set(config.psk_cc_tx_fe_frequency, val);
    }
    else if (key == "psk_cc_tx_fe_stx1_enable")
    {
        set(config.psk_cc_tx_fe_stx1_enable, val);
    }
    else if (key == "psk_cc_tx_fe_stx1_gain")
    {
        set(config.psk_cc_tx_fe_stx1_gain, val);
    }
    else if (key == "psk_cc_tx_fe_stx1_atten")
    {
        set(config.psk_cc_tx_fe_stx1_atten, val);
    }
    else if (key == "psk_cc_tx_fe_stx2_enable")
    {
        set(config.psk_cc_tx_fe_stx2_enable, val);
    }
    else if (key == "psk_cc_tx_fe_stx2_gain")
    {
        set(config.psk_cc_tx_fe_stx2_gain, val);
    }
    else if (key == "psk_cc_tx_fe_stx2_atten")
    {
        set(config.psk_cc_tx_fe_stx2_atten, val);
    }
    else if (key == "psk_cc_tx_fe_sample_rate")
    {
        set(config.psk_cc_tx_fe_sample_rate, val);
    }
    else if (key == "psk_cc_tx_symbol_rate")
    {
        set(config.psk_cc_tx_symbol_rate, val);
    }
    else if (key == "psk_cc_tx_modulation")
    {
        set(config.psk_cc_tx_modulation, val);
    }
    else if (key == "psk_cc_rx_force_on")
    {
        set(config.psk_cc_rx_force_on, val);
    }
    else if (key == "psk_cc_rx_idle_timeout_s")
    {
        set(config.psk_cc_rx_idle_timeout_s, val);
    }
    else if (key == "psk_cc_rx_low_power_timeout_s")
    {
        set(config.psk_cc_rx_low_power_timeout_s, val);
    }
    else if (key == "psk_cc_rx_gain_mode")
    {
        set(config.psk_cc_rx_gain_mode, val);
    }
    else if (key == "psk_cc_rx_auto_antenna_selection")
    {
        set(config.psk_cc_rx_auto_antenna_selection, val);
    }
    else if (key == "psk_cc_rx_fe_frequency")
    {
        set(config.psk_cc_rx_fe_frequency, val);
    }
    else if (key == "psk_cc_rx_fe_srx1_enable")
    {
        set(config.psk_cc_rx_fe_srx1_enable, val);
    }
    else if (key == "psk_cc_rx_fe_srx1_gain")
    {
        set(config.psk_cc_rx_fe_srx1_gain, val);
    }
    else if (key == "psk_cc_rx_fe_srx1_atten")
    {
        set(config.psk_cc_rx_fe_srx1_atten, val);
    }
    else if (key == "psk_cc_rx_fe_srx2_enable")
    {
        set(config.psk_cc_rx_fe_srx2_enable, val);
    }
    else if (key == "psk_cc_rx_fe_srx2_gain")
    {
        set(config.psk_cc_rx_fe_srx2_gain, val);
    }
    else if (key == "psk_cc_rx_fe_srx2_atten")
    {
        set(config.psk_cc_rx_fe_srx2_atten, val);
    }
    else if (key == "psk_cc_rx_fe_sample_rate")
    {
        set(config.psk_cc_rx_fe_sample_rate, val);
    }
    else if (key == "psk_cc_rx_symbol_rate")
    {
        set(config.psk_cc_rx_symbol_rate, val);
    }
    else if (key == "psk_cc_rx_modulation")
    {
        set(config.psk_cc_rx_modulation, val);
    }
    else if (key == "dvbs2_tx_force_on")
    {
        set(config.dvbs2_tx_force_on, val);
    }
    else if (key == "dvbs2_tx_idle_timeout_s")
    {
        set(config.dvbs2_tx_idle_timeout_s, val);
    }
    else if (key == "dvbs2_tx_fe_frequency")
    {
        set(config.dvbs2_tx_fe_frequency, val);
    }
    else if (key == "dvbs2_tx_fe_gain")
    {
        set(config.dvbs2_tx_fe_gain, val);
    }
    else if (key == "dvbs2_tx_fe_sample_rate")
    {
        set(config.dvbs2_tx_fe_sample_rate, val);
    }
    else if (key == "dvbs2_tx_symbol_rate")
    {
        set(config.dvbs2_tx_symbol_rate, val);
    }
    else if (key == "dvbs2_tx_modulation")
    {
        set(config.dvbs2_tx_modulation, val);
    }
    else if (key == "dvbs2_tx_coding")
    {
        set(config.dvbs2_tx_coding, val);
    }
    else if (key == "dvbs2_tx_rolloff")
    {
        set(config.dvbs2_tx_rolloff, val);
    }
    else if (key == "dvbs2_tx_frame_length")
    {
        set(config.dvbs2_tx_frame_length, val);
    }
    else if (key == "dvbs2_tx_signal_scaling")
    {
        set(config.dvbs2_tx_signal_scaling, val);
    }
    else if (key == "gfsk_tx_force_on")
    {
        set(config.gfsk_tx_force_on, val);
    }
    else if (key == "gfsk_tx_idle_timeout_s")
    {
        set(config.gfsk_tx_idle_timeout_s, val);
    }
    else if (key == "gfsk_tx_fe_frequency")
    {
        set(config.gfsk_tx_fe_frequency, val);
    }
    else if (key == "gfsk_tx_fe_gain")
    {
        set(config.gfsk_tx_fe_gain, val);
    }
    else if (key == "gfsk_tx_fe_atten")
    {
        set(config.gfsk_tx_fe_atten, val);
    }
    else if (key == "gfsk_tx_fe_sample_rate")
    {
        set(config.gfsk_tx_fe_sample_rate, val);
    }
    else if (key == "gfsk_tx_symbol_rate")
    {
        set(config.gfsk_tx_symbol_rate, val);
    }
    else if (key == "gfsk_tx_mod_index")
    {
        set(config.gfsk_tx_mod_index, val);
    }
    else if (key == "gfsk_tx_max_payload_len")
    {
        set(config.gfsk_tx_max_payload_len, val);
    }
    else if (key == "gfsk_tx_bt")
    {
        set(config.gfsk_tx_bt, val);
    }
    else if (key == "anylink_active_tx_channel")
    {
        set(config.anylink_active_tx_channel, val);
    }
    else {
        std::cout << "Invalid <key>: " << key << std::endl;
        return;
    }

    std::cout << key << " set to " << val << std::endl;
}

void display(std::string what)
{
    if (what.empty())
    {
        std::cout << "Missing argument to display command" << std::endl;
        return;
    }
    else if (what == "config")
    {
        std::cout << "sharemap_control_url = " << sharemap_control_url << std::endl;
        std::cout << "sharemap_metrics_url = " << sharemap_metrics_url << std::endl;
        std::cout << "source_id = " << to_string(config.source_id) << std::endl;
        std::cout << "schema_hash = " << to_string(config.schema_hash) << std::endl;
        std::cout << "unix_timestamp_ns = " << to_string(config.unix_timestamp_ns) << std::endl;
        std::cout << "psk_cc_tx_force_on = " << to_string(config.psk_cc_tx_force_on) << std::endl;
        std::cout << "psk_cc_tx_idle_timeout_s = " << to_string(config.psk_cc_tx_idle_timeout_s) << std::endl;
        std::cout << "psk_cc_tx_fe_frequency = " << to_string(config.psk_cc_tx_fe_frequency) << std::endl;
        std::cout << "psk_cc_tx_fe_stx1_enable = " << to_string(config.psk_cc_tx_fe_stx1_enable) << std::endl;
        std::cout << "psk_cc_tx_fe_stx1_gain = " << to_string(config.psk_cc_tx_fe_stx1_gain) << std::endl;
        std::cout << "psk_cc_tx_fe_stx1_atten = " << to_string(config.psk_cc_tx_fe_stx1_atten) << std::endl;
        std::cout << "psk_cc_tx_fe_stx2_enable = " << to_string(config.psk_cc_tx_fe_stx2_enable) << std::endl;
        std::cout << "psk_cc_tx_fe_stx2_gain = " << to_string(config.psk_cc_tx_fe_stx2_gain) << std::endl;
        std::cout << "psk_cc_tx_fe_stx2_atten = " << to_string(config.psk_cc_tx_fe_stx2_atten) << std::endl;
        std::cout << "psk_cc_tx_fe_sample_rate = " << to_string(config.psk_cc_tx_fe_sample_rate) << std::endl;
        std::cout << "psk_cc_tx_symbol_rate = " << to_string(config.psk_cc_tx_symbol_rate) << std::endl;
        std::cout << "psk_cc_tx_modulation = " << to_string(config.psk_cc_tx_modulation) << std::endl;
        std::cout << "psk_cc_rx_force_on = " << to_string(config.psk_cc_rx_force_on) << std::endl;
        std::cout << "psk_cc_rx_idle_timeout_s = " << to_string(config.psk_cc_rx_idle_timeout_s) << std::endl;
        std::cout << "psk_cc_rx_low_power_timeout_s = " << to_string(config.psk_cc_rx_low_power_timeout_s) << std::endl;
        std::cout << "psk_cc_rx_gain_mode = " << to_string(config.psk_cc_rx_gain_mode) << std::endl;
        std::cout << "psk_cc_rx_auto_antenna_selection = " << to_string(config.psk_cc_rx_auto_antenna_selection) << std::endl;
        std::cout << "psk_cc_rx_fe_frequency = " << to_string(config.psk_cc_rx_fe_frequency) << std::endl;
        std::cout << "psk_cc_rx_fe_srx1_enable = " << to_string(config.psk_cc_rx_fe_srx1_enable) << std::endl;
        std::cout << "psk_cc_rx_fe_srx1_gain = " << to_string(config.psk_cc_rx_fe_srx1_gain) << std::endl;
        std::cout << "psk_cc_rx_fe_srx1_atten = " << to_string(config.psk_cc_rx_fe_srx1_atten) << std::endl;
        std::cout << "psk_cc_rx_fe_srx2_enable = " << to_string(config.psk_cc_rx_fe_srx2_enable) << std::endl;
        std::cout << "psk_cc_rx_fe_srx2_gain = " << to_string(config.psk_cc_rx_fe_srx2_gain) << std::endl;
        std::cout << "psk_cc_rx_fe_srx2_atten = " << to_string(config.psk_cc_rx_fe_srx2_atten) << std::endl;
        std::cout << "psk_cc_rx_fe_sample_rate = " << to_string(config.psk_cc_rx_fe_sample_rate) << std::endl;
        std::cout << "psk_cc_rx_symbol_rate = " << to_string(config.psk_cc_rx_symbol_rate) << std::endl;
        std::cout << "psk_cc_rx_modulation = " << to_string(config.psk_cc_rx_modulation) << std::endl;
        std::cout << "dvbs2_tx_force_on = " << to_string(config.dvbs2_tx_force_on) << std::endl;
        std::cout << "dvbs2_tx_idle_timeout_s = " << to_string(config.dvbs2_tx_idle_timeout_s) << std::endl;
        std::cout << "dvbs2_tx_fe_frequency = " << to_string(config.dvbs2_tx_fe_frequency) << std::endl;
        std::cout << "dvbs2_tx_fe_gain = " << to_string(config.dvbs2_tx_fe_gain) << std::endl;
        std::cout << "dvbs2_tx_fe_sample_rate = " << to_string(config.dvbs2_tx_fe_sample_rate) << std::endl;
        std::cout << "dvbs2_tx_symbol_rate = " << to_string(config.dvbs2_tx_symbol_rate) << std::endl;
        std::cout << "dvbs2_tx_modulation = " << to_string(config.dvbs2_tx_modulation) << std::endl;
        std::cout << "dvbs2_tx_coding = " << to_string(config.dvbs2_tx_coding) << std::endl;
        std::cout << "dvbs2_tx_rolloff = " << to_string(config.dvbs2_tx_rolloff) << std::endl;
        std::cout << "dvbs2_tx_frame_length = " << to_string(config.dvbs2_tx_frame_length) << std::endl;
        std::cout << "dvbs2_tx_signal_scaling = " << to_string(config.dvbs2_tx_signal_scaling) << std::endl;
        std::cout << "gfsk_tx_force_on = " << to_string(config.gfsk_tx_force_on) << std::endl;
        std::cout << "gfsk_tx_idle_timeout_s = " << to_string(config.gfsk_tx_idle_timeout_s) << std::endl;
        std::cout << "gfsk_tx_fe_frequency = " << to_string(config.gfsk_tx_fe_frequency) << std::endl;
        std::cout << "gfsk_tx_fe_gain = " << to_string(config.gfsk_tx_fe_gain) << std::endl;
        std::cout << "gfsk_tx_fe_atten = " << to_string(config.gfsk_tx_fe_atten) << std::endl;
        std::cout << "gfsk_tx_fe_sample_rate = " << to_string(config.gfsk_tx_fe_sample_rate) << std::endl;
        std::cout << "gfsk_tx_symbol_rate = " << to_string(config.gfsk_tx_symbol_rate) << std::endl;
        std::cout << "gfsk_tx_mod_index = " << to_string(config.gfsk_tx_mod_index) << std::endl;
        std::cout << "gfsk_tx_max_payload_len = " << to_string(config.gfsk_tx_max_payload_len) << std::endl;
        std::cout << "gfsk_tx_bt = " << to_string(config.gfsk_tx_bt) << std::endl;
        std::cout << "anylink_active_tx_channel = " << to_string(config.anylink_active_tx_channel) << std::endl;
    }
    else if (what == "metrics")
    {
        if (!metrics_initialized)
        {
            std::cout << "No metrics received" << std::endl;
            return;
        }
        std::cout << "source_id = " << to_string(metrics.source_id) << std::endl;
        std::cout << "schema_hash = " << to_string(metrics.schema_hash) << std::endl;
        std::cout << "unix_timestamp_ns = " << to_string(metrics.unix_timestamp_ns) << std::endl;
        std::cout << "controld_version = " << to_string(metrics.controld_version) << std::endl;
        std::cout << "controld_timestamp = " << to_string(metrics.controld_timestamp) << std::endl;
        std::cout << "powerd_version = " << to_string(metrics.powerd_version) << std::endl;
        std::cout << "powerd_timestamp = " << to_string(metrics.powerd_timestamp) << std::endl;
        std::cout << "radiod_version = " << to_string(metrics.radiod_version) << std::endl;
        std::cout << "radiod_timestamp = " << to_string(metrics.radiod_timestamp) << std::endl;
        std::cout << "fpga_version = " << to_string(metrics.fpga_version) << std::endl;
        std::cout << "fpga_timestamp = " << to_string(metrics.fpga_timestamp) << std::endl;
        std::cout << "fpga_project_name = " << to_string(metrics.fpga_project_name) << std::endl;
        std::cout << "anylink_version = " << to_string(metrics.anylink_version) << std::endl;
        std::cout << "psk_cc_tx_bytes_total = " << to_string(metrics.psk_cc_tx_bytes_total) << std::endl;
        std::cout << "psk_cc_tx_underflows = " << to_string(metrics.psk_cc_tx_underflows) << std::endl;
        std::cout << "psk_cc_tx_client_recv_errors = " << to_string(metrics.psk_cc_tx_client_recv_errors) << std::endl;
        std::cout << "psk_cc_tx_client_msgs = " << to_string(metrics.psk_cc_tx_client_msgs) << std::endl;
        std::cout << "psk_cc_tx_frames_transmitted = " << to_string(metrics.psk_cc_tx_frames_transmitted) << std::endl;
        std::cout << "psk_cc_tx_failed_transmissions = " << to_string(metrics.psk_cc_tx_failed_transmissions) << std::endl;
        std::cout << "psk_cc_tx_dropped_packets = " << to_string(metrics.psk_cc_tx_dropped_packets) << std::endl;
        std::cout << "psk_cc_tx_idle_frames_transmitted = " << to_string(metrics.psk_cc_tx_idle_frames_transmitted) << std::endl;
        std::cout << "psk_cc_tx_failed_idle_frames_transmitted = " << to_string(metrics.psk_cc_tx_failed_idle_frames_transmitted) << std::endl;
        std::cout << "psk_cc_tx_failed_bytes_in_flight_checks = " << to_string(metrics.psk_cc_tx_failed_bytes_in_flight_checks) << std::endl;
        std::cout << "psk_cc_tx_modem_underflows = " << to_string(metrics.psk_cc_tx_modem_underflows) << std::endl;
        std::cout << "psk_cc_tx_ad9361_tx_pll_lock = " << to_string(metrics.psk_cc_tx_ad9361_tx_pll_lock) << std::endl;
        std::cout << "psk_cc_rx_bytes_total = " << to_string(metrics.psk_cc_rx_bytes_total) << std::endl;
        std::cout << "psk_cc_rx_client_send_errors = " << to_string(metrics.psk_cc_rx_client_send_errors) << std::endl;
        std::cout << "psk_cc_rx_client_msgs = " << to_string(metrics.psk_cc_rx_client_msgs) << std::endl;
        std::cout << "psk_cc_rx_frames_received = " << to_string(metrics.psk_cc_rx_frames_received) << std::endl;
        std::cout << "psk_cc_rx_failed_receptions = " << to_string(metrics.psk_cc_rx_failed_receptions) << std::endl;
        std::cout << "psk_cc_rx_dropped_good_packets = " << to_string(metrics.psk_cc_rx_dropped_good_packets) << std::endl;
        std::cout << "psk_cc_rx_failed_frames_available_checks = " << to_string(metrics.psk_cc_rx_failed_frames_available_checks) << std::endl;
        std::cout << "psk_cc_rx_encountered_frames_in_progress = " << to_string(metrics.psk_cc_rx_encountered_frames_in_progress) << std::endl;
        std::cout << "psk_cc_rx_modem_dma_overflows = " << to_string(metrics.psk_cc_rx_modem_dma_overflows) << std::endl;
        std::cout << "psk_cc_rx_modem_dma_packet_count = " << to_string(metrics.psk_cc_rx_modem_dma_packet_count) << std::endl;
        std::cout << "psk_cc_rx_signal_present = " << to_string(metrics.psk_cc_rx_signal_present) << std::endl;
        std::cout << "psk_cc_rx_carrier_lock = " << to_string(metrics.psk_cc_rx_carrier_lock) << std::endl;
        std::cout << "psk_cc_rx_frame_sync_lock = " << to_string(metrics.psk_cc_rx_frame_sync_lock) << std::endl;
        std::cout << "psk_cc_rx_fec_confirmed_lock = " << to_string(metrics.psk_cc_rx_fec_confirmed_lock) << std::endl;
        std::cout << "psk_cc_rx_fec_ber = " << to_string(metrics.psk_cc_rx_fec_ber) << std::endl;
        std::cout << "psk_cc_rx_ad9361_rx_pll_lock = " << to_string(metrics.psk_cc_rx_ad9361_rx_pll_lock) << std::endl;
        std::cout << "psk_cc_rx_ad9361_bb_pll_lock = " << to_string(metrics.psk_cc_rx_ad9361_bb_pll_lock) << std::endl;
        std::cout << "dvbs2_tx_bytes_total = " << to_string(metrics.dvbs2_tx_bytes_total) << std::endl;
        std::cout << "dvbs2_tx_underflows = " << to_string(metrics.dvbs2_tx_underflows) << std::endl;
        std::cout << "dvbs2_tx_client_recv_errors = " << to_string(metrics.dvbs2_tx_client_recv_errors) << std::endl;
        std::cout << "dvbs2_tx_client_msgs = " << to_string(metrics.dvbs2_tx_client_msgs) << std::endl;
        std::cout << "dvbs2_tx_frames_transmitted = " << to_string(metrics.dvbs2_tx_frames_transmitted) << std::endl;
        std::cout << "dvbs2_tx_failed_transmissions = " << to_string(metrics.dvbs2_tx_failed_transmissions) << std::endl;
        std::cout << "dvbs2_tx_dropped_packets = " << to_string(metrics.dvbs2_tx_dropped_packets) << std::endl;
        std::cout << "dvbs2_tx_idle_frames_transmitted = " << to_string(metrics.dvbs2_tx_idle_frames_transmitted) << std::endl;
        std::cout << "dvbs2_tx_failed_idle_frames_transmitted = " << to_string(metrics.dvbs2_tx_failed_idle_frames_transmitted) << std::endl;
        std::cout << "dvbs2_tx_failed_bytes_in_flight_checks = " << to_string(metrics.dvbs2_tx_failed_bytes_in_flight_checks) << std::endl;
        std::cout << "dvbs2_tx_dummy_pl_frames = " << to_string(metrics.dvbs2_tx_dummy_pl_frames) << std::endl;
        std::cout << "gfsk_tx_bytes_total = " << to_string(metrics.gfsk_tx_bytes_total) << std::endl;
        std::cout << "gfsk_tx_underflows = " << to_string(metrics.gfsk_tx_underflows) << std::endl;
        std::cout << "gfsk_tx_client_recv_errors = " << to_string(metrics.gfsk_tx_client_recv_errors) << std::endl;
        std::cout << "gfsk_tx_client_msgs = " << to_string(metrics.gfsk_tx_client_msgs) << std::endl;
        std::cout << "gfsk_tx_frames_transmitted = " << to_string(metrics.gfsk_tx_frames_transmitted) << std::endl;
        std::cout << "gfsk_tx_failed_transmissions = " << to_string(metrics.gfsk_tx_failed_transmissions) << std::endl;
        std::cout << "gfsk_tx_dropped_packets = " << to_string(metrics.gfsk_tx_dropped_packets) << std::endl;
        std::cout << "gfsk_tx_idle_frames_transmitted = " << to_string(metrics.gfsk_tx_idle_frames_transmitted) << std::endl;
        std::cout << "gfsk_tx_failed_idle_frames_transmitted = " << to_string(metrics.gfsk_tx_failed_idle_frames_transmitted) << std::endl;
        std::cout << "gfsk_tx_failed_bytes_in_flight_checks = " << to_string(metrics.gfsk_tx_failed_bytes_in_flight_checks) << std::endl;
        std::cout << "ad9122_pgood = " << to_string(metrics.ad9122_pgood) << std::endl;
        std::cout << "ad9361_pgood = " << to_string(metrics.ad9361_pgood) << std::endl;
        std::cout << "adrf6780_pgood = " << to_string(metrics.adrf6780_pgood) << std::endl;
        std::cout << "at86_pgood = " << to_string(metrics.at86_pgood) << std::endl;
        std::cout << "at86_is_pll_locked = " << to_string(metrics.at86_is_pll_locked) << std::endl;
        std::cout << "aux_3v8_isense = " << to_string(metrics.aux_3v8_isense) << std::endl;
        std::cout << "aux_3v8_vsense = " << to_string(metrics.aux_3v8_vsense) << std::endl;
        std::cout << "carrier_28v0_isense = " << to_string(metrics.carrier_28v0_isense) << std::endl;
        std::cout << "carrier_28v0_vsense = " << to_string(metrics.carrier_28v0_vsense) << std::endl;
        std::cout << "carrier_2v1_isense = " << to_string(metrics.carrier_2v1_isense) << std::endl;
        std::cout << "carrier_2v1_vsense = " << to_string(metrics.carrier_2v1_vsense) << std::endl;
        std::cout << "carrier_2v6_isense = " << to_string(metrics.carrier_2v6_isense) << std::endl;
        std::cout << "carrier_2v6_vsense = " << to_string(metrics.carrier_2v6_vsense) << std::endl;
        std::cout << "carrier_3v8_isense = " << to_string(metrics.carrier_3v8_isense) << std::endl;
        std::cout << "carrier_3v8_vsense = " << to_string(metrics.carrier_3v8_vsense) << std::endl;
        std::cout << "carrier_5v5_isense = " << to_string(metrics.carrier_5v5_isense) << std::endl;
        std::cout << "carrier_5v5_vsense = " << to_string(metrics.carrier_5v5_vsense) << std::endl;
        std::cout << "carrier_temp = " << to_string(metrics.carrier_temp) << std::endl;
        std::cout << "lband_rx_pgood = " << to_string(metrics.lband_rx_pgood) << std::endl;
        std::cout << "lband_temp = " << to_string(metrics.lband_temp) << std::endl;
        std::cout << "lband_tx_pgood = " << to_string(metrics.lband_tx_pgood) << std::endl;
        std::cout << "lband_tx_rf_detect = " << to_string(metrics.lband_tx_rf_detect) << std::endl;
        std::cout << "lmk04832_pgood = " << to_string(metrics.lmk04832_pgood) << std::endl;
        std::cout << "lmk04832_is_pll_locked = " << to_string(metrics.lmk04832_is_pll_locked) << std::endl;
        std::cout << "lmx2594_pgood = " << to_string(metrics.lmx2594_pgood) << std::endl;
        std::cout << "max2771_a_1_is_pll_locked = " << to_string(metrics.max2771_a_1_is_pll_locked) << std::endl;
        std::cout << "max2771_a_2_is_pll_locked = " << to_string(metrics.max2771_a_2_is_pll_locked) << std::endl;
        std::cout << "max2771_a_bias_pgood = " << to_string(metrics.max2771_a_bias_pgood) << std::endl;
        std::cout << "max2771_a_pgood = " << to_string(metrics.max2771_a_pgood) << std::endl;
        std::cout << "max2771_b_1_is_pll_locked = " << to_string(metrics.max2771_b_1_is_pll_locked) << std::endl;
        std::cout << "max2771_b_2_is_pll_locked = " << to_string(metrics.max2771_b_2_is_pll_locked) << std::endl;
        std::cout << "max2771_b_bias_pgood = " << to_string(metrics.max2771_b_bias_pgood) << std::endl;
        std::cout << "max2771_b_pgood = " << to_string(metrics.max2771_b_pgood) << std::endl;
        std::cout << "rf_fe_mux_pgood = " << to_string(metrics.rf_fe_mux_pgood) << std::endl;
        std::cout << "sband_rx_pgood = " << to_string(metrics.sband_rx_pgood) << std::endl;
        std::cout << "sband_temp = " << to_string(metrics.sband_temp) << std::endl;
        std::cout << "sband_tx_pgood = " << to_string(metrics.sband_tx_pgood) << std::endl;
        std::cout << "sband_tx_rf_detect = " << to_string(metrics.sband_tx_rf_detect) << std::endl;
        std::cout << "si5345_pgood = " << to_string(metrics.si5345_pgood) << std::endl;
        std::cout << "som_5v0_isense = " << to_string(metrics.som_5v0_isense) << std::endl;
        std::cout << "som_5v0_vsense = " << to_string(metrics.som_5v0_vsense) << std::endl;
        std::cout << "uhf_rx_pgood = " << to_string(metrics.uhf_rx_pgood) << std::endl;
        std::cout << "uhf_temp = " << to_string(metrics.uhf_temp) << std::endl;
        std::cout << "uhf_tx_pgood = " << to_string(metrics.uhf_tx_pgood) << std::endl;
        std::cout << "uhf_tx_rf_detect = " << to_string(metrics.uhf_tx_rf_detect) << std::endl;
        std::cout << "xband_24v0_isense = " << to_string(metrics.xband_24v0_isense) << std::endl;
        std::cout << "xband_24v0_vsense = " << to_string(metrics.xband_24v0_vsense) << std::endl;
        std::cout << "xband_drain_pgood = " << to_string(metrics.xband_drain_pgood) << std::endl;
        std::cout << "xband_temp = " << to_string(metrics.xband_temp) << std::endl;
        std::cout << "xband_tx_rf_detect = " << to_string(metrics.xband_tx_rf_detect) << std::endl;
        std::cout << "anylink_uhf_tx_sent_bytes = " << to_string(metrics.anylink_uhf_tx_sent_bytes) << std::endl;
        std::cout << "anylink_uhf_tx_sent_packets = " << to_string(metrics.anylink_uhf_tx_sent_packets) << std::endl;
        std::cout << "anylink_uhf_tx_sent_frames = " << to_string(metrics.anylink_uhf_tx_sent_frames) << std::endl;
        std::cout << "anylink_uhf_tx_overflow_frames = " << to_string(metrics.anylink_uhf_tx_overflow_frames) << std::endl;
        std::cout << "anylink_sband_tx_sent_bytes = " << to_string(metrics.anylink_sband_tx_sent_bytes) << std::endl;
        std::cout << "anylink_sband_tx_sent_packets = " << to_string(metrics.anylink_sband_tx_sent_packets) << std::endl;
        std::cout << "anylink_sband_tx_sent_frames = " << to_string(metrics.anylink_sband_tx_sent_frames) << std::endl;
        std::cout << "anylink_sband_tx_overflow_frames = " << to_string(metrics.anylink_sband_tx_overflow_frames) << std::endl;
        std::cout << "anylink_xband_tx_sent_bytes = " << to_string(metrics.anylink_xband_tx_sent_bytes) << std::endl;
        std::cout << "anylink_xband_tx_sent_packets = " << to_string(metrics.anylink_xband_tx_sent_packets) << std::endl;
        std::cout << "anylink_xband_tx_sent_frames = " << to_string(metrics.anylink_xband_tx_sent_frames) << std::endl;
        std::cout << "anylink_xband_tx_overflow_frames = " << to_string(metrics.anylink_xband_tx_overflow_frames) << std::endl;
        std::cout << "anylink_sband_rx_received_bytes = " << to_string(metrics.anylink_sband_rx_received_bytes) << std::endl;
        std::cout << "anylink_sband_rx_received_packets = " << to_string(metrics.anylink_sband_rx_received_packets) << std::endl;
        std::cout << "anylink_sband_rx_received_frames = " << to_string(metrics.anylink_sband_rx_received_frames) << std::endl;
        std::cout << "anylink_sband_rx_dropped_packets = " << to_string(metrics.anylink_sband_rx_dropped_packets) << std::endl;
        std::cout << "anylink_sband_rx_dropped_frames = " << to_string(metrics.anylink_sband_rx_dropped_frames) << std::endl;
        std::cout << "anylink_sband_rx_socket_errors = " << to_string(metrics.anylink_sband_rx_socket_errors) << std::endl;
        std::cout << "anylink_sband_rx_idle_frames = " << to_string(metrics.anylink_sband_rx_idle_frames) << std::endl;
        std::cout << "anylink_heartbeats_sent = " << to_string(metrics.anylink_heartbeats_sent) << std::endl;
        std::cout << "anylink_heartbeats_received = " << to_string(metrics.anylink_heartbeats_received) << std::endl;
        std::cout << "anylink_rx_radio_bad_header = " << to_string(metrics.anylink_rx_radio_bad_header) << std::endl;
        std::cout << "anylink_rx_radio_packets_received = " << to_string(metrics.anylink_rx_radio_packets_received) << std::endl;
        std::cout << "anylink_tx_radio_packets_send_errors = " << to_string(metrics.anylink_tx_radio_packets_send_errors) << std::endl;
        std::cout << "anylink_tx_radio_packets_sent = " << to_string(metrics.anylink_tx_radio_packets_sent) << std::endl;
        std::cout << "anylink_tx_radio_packet_nodest = " << to_string(metrics.anylink_tx_radio_packet_nodest) << std::endl;
        std::cout << "anylink_tx_radio_packet_truncate = " << to_string(metrics.anylink_tx_radio_packet_truncate) << std::endl;
        std::cout << "anylink_tx_radio_packet_pad = " << to_string(metrics.anylink_tx_radio_packet_pad) << std::endl;
        std::cout << "anylink_rx_radio_no_endpoint = " << to_string(metrics.anylink_rx_radio_no_endpoint) << std::endl;
        std::cout << "anylink_rx_radio_reject_echo = " << to_string(metrics.anylink_rx_radio_reject_echo) << std::endl;
        std::cout << "anylink_total_endpoint_packets_received = " << to_string(metrics.anylink_total_endpoint_packets_received) << std::endl;
        std::cout << "anylink_total_endpoint_packets_sent = " << to_string(metrics.anylink_total_endpoint_packets_sent) << std::endl;
        std::cout << "anylink_encryption_failed = " << to_string(metrics.anylink_encryption_failed) << std::endl;
        std::cout << "anylink_decryption_failed = " << to_string(metrics.anylink_decryption_failed) << std::endl;
        std::cout << "anylink_tap_endpoint_active_tx_channel = " << to_string(metrics.anylink_tap_endpoint_active_tx_channel) << std::endl;
        std::cout << "anylink_tap_endpoint_mtu = " << to_string(metrics.anylink_tap_endpoint_mtu) << std::endl;
        std::cout << "anylink_tap_endpoint_recv_bytes = " << to_string(metrics.anylink_tap_endpoint_recv_bytes) << std::endl;
        std::cout << "anylink_tap_endpoint_recv_errors = " << to_string(metrics.anylink_tap_endpoint_recv_errors) << std::endl;
        std::cout << "anylink_tap_endpoint_recv_packets = " << to_string(metrics.anylink_tap_endpoint_recv_packets) << std::endl;
        std::cout << "anylink_tap_endpoint_send_bytes = " << to_string(metrics.anylink_tap_endpoint_send_bytes) << std::endl;
        std::cout << "anylink_tap_endpoint_send_errors = " << to_string(metrics.anylink_tap_endpoint_send_errors) << std::endl;
        std::cout << "anylink_tap_endpoint_send_packets = " << to_string(metrics.anylink_tap_endpoint_send_packets) << std::endl;
    }
    else
    {
        std::cout << "Invalid argument to display command: " << what << std::endl;
    }
}

void connect()
{
    std::cout << "Connecting sharemap client" << std::endl;
    if (metrics_socket)
    {
        std::cout << "Already connected" << std::endl;
        return;
    }
    control_socket = new anysignal::udp_sock();
    control_socket->connect(sharemap_control_url);
    metrics_socket = new anysignal::udp_sock();
    metrics_socket->bind(sharemap_metrics_url);

    std::cout << "Starting metrics monitor" << std::endl;
    receiving = true;
    recv_thread = std::thread(recv_metrics);
}

void disconnect()
{
    std::cout << "Disonnecting sharemap client" << std::endl;
    if (!metrics_socket)
    {
        std::cout << "No connection detected" << std::endl;
        return;
    }
    receiving = false;
    if (recv_thread.joinable())
    {
        recv_thread.join();
    }
    delete control_socket;
    control_socket = nullptr;
    delete metrics_socket;
    metrics_socket = nullptr;
    metrics_initialized = false;
}

void send_cmd(std::string arg)
{
    if (arg == "config")
    {
        std::cout << "Sending config" << std::endl;
        if (!control_socket)
        {
            std::cout << "No control socket connected" << std::endl;
            return;
        }
        auto packed_config = anysignal::sharemap_pack(config);
        control_socket->send(reinterpret_cast<uint8_t *>(&packed_config), anysignal::sharemap_config_t::PACKED_SIZE);
        std::cout << "Config sent" << std::endl;
    }
    else
    {
        std::cout << "invalid argument to send command: " << arg << std::endl;
    }
}

int main(int /*argc*/, char * /*argv*/[])
{
    // Register signal handler
    signal(SIGINT, signal_callback_handler);
    signal(SIGTERM, signal_callback_handler);

    // Initial config
    std::string val;
    config.psk_cc_tx_force_on = false;
    config.psk_cc_tx_idle_timeout_s = 4;
    config.psk_cc_tx_fe_frequency = 2.25e9;
    config.psk_cc_tx_fe_sample_rate = 30.72e6;
    config.psk_cc_tx_fe_stx1_enable = true;
    config.psk_cc_tx_fe_stx1_gain = 55;
    config.psk_cc_tx_fe_stx1_atten = 0;
    config.psk_cc_tx_fe_stx2_enable = false;
    config.psk_cc_tx_fe_stx2_gain = 55;
    config.psk_cc_tx_fe_stx2_atten = 0;
    config.psk_cc_tx_symbol_rate = 960e3;
    config.psk_cc_rx_force_on = true;
    config.psk_cc_rx_idle_timeout_s = 4;
    config.psk_cc_rx_low_power_timeout_s = 1;
    val = "MANUAL";
    std::copy(val.begin(), val.end(), config.psk_cc_rx_gain_mode.data());
    config.psk_cc_rx_auto_antenna_selection = false;
    config.psk_cc_rx_fe_frequency = 2.053e9;
    config.psk_cc_rx_fe_sample_rate = 30.72e6;
    config.psk_cc_rx_fe_srx1_enable = true;
    config.psk_cc_rx_fe_srx1_gain = 40;
    config.psk_cc_rx_fe_srx1_atten = 0;
    config.psk_cc_rx_fe_srx2_enable = false;
    config.psk_cc_rx_fe_srx2_gain = 40;
    config.psk_cc_rx_fe_srx2_atten = 0;
    config.psk_cc_rx_fe_sample_rate = 30.72e6;
    config.psk_cc_rx_symbol_rate = 960e3;
    config.dvbs2_tx_force_on = false;
    config.dvbs2_tx_idle_timeout_s = 4;
    config.dvbs2_tx_fe_frequency = 8.488e9;
    config.dvbs2_tx_fe_gain = 69;
    config.dvbs2_tx_fe_sample_rate = 30.72e6;
    config.dvbs2_tx_symbol_rate = 3.84e6;
    val = "QPSK";
    std::copy(val.begin(), val.end(), config.dvbs2_tx_modulation.data());
    val = "1/4";
    std::copy(val.begin(), val.end(), config.dvbs2_tx_coding.data());
    val = "35%";
    std::copy(val.begin(), val.end(), config.dvbs2_tx_rolloff.data());
    val = "NORMAL";
    std::copy(val.begin(), val.end(), config.dvbs2_tx_frame_length.data());
    config.dvbs2_tx_signal_scaling = 1.0;
    config.gfsk_tx_force_on = false;
    config.gfsk_tx_idle_timeout_s = 10;
    config.gfsk_tx_fe_frequency = 401.5e6;
    config.gfsk_tx_fe_gain = 10;
    config.gfsk_tx_fe_atten = 0;
    config.gfsk_tx_fe_sample_rate = 400e3;
    config.gfsk_tx_symbol_rate = 9.6e3;
    config.gfsk_tx_mod_index = 0.5;
    config.gfsk_tx_max_payload_len = 128;
    config.gfsk_tx_bt = 1.0;
    val = "tx_sband";
    std::copy(val.begin(), val.end(), config.anylink_active_tx_channel.data());

    std::string in;
    std::string command;
    std::string args;
    while (running)
    {
        std::cout << "> ";
        std::getline(std::cin, in);
        if (std::cin.eof()) break; // stdin closed by signal handler
        auto pos = in.find_first_of(' ');
        if (pos == std::string::npos)
        {
            command = in;
            args = "";
        }
        else
        {
            command = in.substr(0, pos);
            args = in.substr(pos + 1);
        }
        if (command == "help" or command == "?")
        {
            help();
        }
        else if (command == "set")
        {
            set(args);
        }
        else if (command == "display")
        {
            display(args);
        }
        else if (command == "set")
        {
            set(args);
        }
        else if (command == "connect")
        {
            connect();
        }
        else if (command == "disconnect")
        {
            disconnect();
        }
        else if (command == "send")
        {
            send_cmd(args);
        }
        else if (command == "quit")
        {
            running = false;
        }
        else
        {
            std::cout << "Invalid command: " << command << std::endl;
            help();
        }
    }

    if (recv_thread.joinable())
    {
        receiving = false;
        recv_thread.join();
    }

    return EXIT_SUCCESS;
}