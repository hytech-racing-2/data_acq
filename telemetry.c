#include <stdlib.h>
#include <stdio.h>
#include "../Libraries/HyTech_CAN/HyTech_CAN.h"
#include <MQTTClient.h>

/**
 * This is an exact translation of the Python code. It doesn't precisely match
 * what I'm reading online about Fletcher's checksum (where for example does the
 * constant 5802 come from?), but I suppose that the Python code is what worked.
 */
static short fletcher16(const void *data, const size_t len)
{
    const char *bytes = data;
    short c0 = 0, c1 = 0;
    size_t rem = len;
    const char *b = bytes;
    while (rem >= 5802) {
        for (int i = 0; i < 5802; i++) {
            c0 += *b++;
            c1 += c0;
        }
        c0 %= 255;
        c1 %= 255;
        rem -= 5802;
    }

    for (b = bytes; b < bytes + len; b++) {
        c0 += *b;
        c1 += c0;
    }
    c0 %= 255;
    c1 %= 255;

    return (c1 << 8) | c0;
}

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

struct {
    CAN_msg_rcu_status                      rcu_status;
    CAN_msg_fcu_status                      fcu_status;
    CAN_msg_fcu_readings                    fcu_readings;
    CAN_message_bms_voltages_t              bms_voltages;
    CAN_message_bms_detailed_voltages_t     bms_detailed_voltages;
    CAN_message_bms_temperatures_t          bms_temperatures;
    CAN_message_bms_detailed_temperatures_t bms_detailed_temperatures;
    CAN_message_bms_onboard_temperatures_t  bms_onboard_temperatures;
    CAN_message_bms_onboard_detailed_temperatures_t bms_onboard_detailed_temperatures;
    CAN_message_bms_status_t                bms_status;
    CAN_message_bms_balancing_status_t      bms_balancing_status;
    CAN_message_ccu_status_t                ccu_status;
    CAN_message_mc_temperatures_1_t         mc_temperatures_1;
    CAN_message_mc_temperatures_2_t         mc_temperatures_2;
    CAN_message_mc_temperatures_3_t         mc_temperatures_3;
    CAN_message_mc_analog_input_voltages_t  mc_analog_input_voltages;
    CAN_message_mc_digital_input_status_t   mc_digital_input_status;
    CAN_message_mc_motor_position_information_t mc_motor_position_information;
    CAN_message_mc_current_information_t    mc_current_information;
    CAN_message_mc_voltage_information_t    mc_voltage_information;
    CAN_message_mc_internal_states_t        mc_internal_states;
    CAN_message_mc_fault_codes_t            mc_fault_codes;
    CAN_message_mc_torque_timer_information_t mc_torque_timer_information;
    CAN_message_mc_modulation_index_flux_weakening_output_information_t
        mc_modulation_index_flux_weakening_output_information;
    CAN_message_mc_firmware_information_t   mc_firmware_information;
    CAN_message_mc_command_message_t        mc_command_message;
    CAN_message_mc_read_write_parameter_command_t
        mc_read_write_parameter_command;
    CAN_message_mc_read_write_parameter_response_t
        mc_read_write_parameter_response;
    CAN_message_fcu_accelerometer_values_t  fcu_accelerometer_values;
} current_status;

static void process_message(uint64_t timestamp, CAN_message_t *msg)
{
    // Do logging stuff.
    
    switch (msg->msg_id) {
        case ID_RCU_STATUS:
        {
            CAN_message_rcu_status_t *data = &msg->contents.rcu_status;
            printf("RCU STATE: %hhu\n"
                   "RCU FLAGS: 0x%hhX\n"
                   "GLV BATT VOLTAGE: %f\n"
                   "RCU BMS FAULT: %hhu\n"
                   "RCU IMD FAULT: %hhu\n",
                   data->state,
                   data->glv_battery_voltage,
                   (char)(!(data->flags & 1)),
                   (char)(!(data->flags & 2)));
            current_status.rcu_status = *data;
            break;
        }
        case ID_FCU_STATUS:
        {
            CAN_message_fcu_status_t *data = &msg->contents.fcu_status;
            printf("FCU STATE: %hhu\n"
                   "FCU FLAGS: %hhX\n"
                   "FCU START BUTTON ID: %hhu\n"
                   "FCU BRAKE ACT: %hhu\n"
                   "FCU IMPLAUS ACCEL: %hhu\n"
                   "FCU IMPLAUSE BRAKE: %hhu\n",
                   data->state,
                   data->flags,
                   data->start_button_press_id,
                   (char)((data->flags & 8) >> 3),
                   (char)(data->flags & 1),
                   (char)((data->flags & 4) >> 2));
            current_status.fcu_status = *data;
            break;
        }
        case ID_FCU_READINGS:
        case ID_FCU_ACCELEROMETER:
        case ID_RCU_RESTART_MC:
        case ID_BMS_ONBOARD_TEMPERATURES:
        case ID_BMS_STATUS:
        case ID_BMS_BALANCING_STATUS:
        case ID_FH_WATCHDOG_TEST:
        case ID_CCU_STATUS:
        case ID_MC_TEMPERATURES_1: {
            CAN_message_mc_temperatures_1_t *data = &msg->contents.mc_temperatures_1;
            printf("[%llu] MODULE A TEMP: %f C\n"
                   "MODULE B TEMP: %f C\n"
                   "MODULE C TEMP: %f C\n"
                   "GATE DRIVER BOARD TEMP: %f C\n",
                    timestamp,
                    data->module_a_temperature / 10.
                    data->module_b_temperature / 10.
                    data->module_c_temperature / 10.,
                    data->gate_driver_board_temperature / 10.);
            current_status.mc_temperatures_1 = *data;
            break;
        }
        case ID_MC_TEMPERATURES_2: {
            CAN_message_mc_temperatures_2_t *data = &msg->contents.mc_temperatures_2;
            printf("CONTROL BOARD TEMP: %f C\n"
                   "RTD 1 TEMP: %f C\n"
                   "RTD 2 TEMP: %f C\n"
                   "RTD 3 TEMP: %f C\n",
                    data->control_board_temperature / 10.
                    data->rtd_1_temperature / 10.
                    data->rtd_2_temperature / 10.,
                    data->rtd_3_temperature / 10.);
            current_status.mc_temperatures_2 = *data;
            break;
        }
        case ID_MC_TEMPERATURES_3: {
            CAN_message_mc_temperatures_3_t *data = &msg->contents.mc_temperatures_3;
            printf("RTD 4 TEMP: %f C\n"
                   "RTD 5 TEMP: %f C\n"
                   "MOTOR TEMP: %f C\n"
                   "TORQUE SHUDDER: %f Nm\n",
                    data->rtd_4_temperature / 10.,
                    data->rtd_5_temperature / 10.,
                    data->motor_temperature / 10.,
                    data->torque_shudder / 10.);
            current_status.mc_temperatures_3 = *data;
            break;
        }
        case ID_MC_ANALOG_INPUTS_VOLTAGES: {
            CAN_mesage_mc_analog_input_voltages_t *data =
                &msg->contents.mc_analog_input_voltages;
            printf();
            current_status.mc_analog_input_voltages = *data;
            break;
        }
        case ID_MC_DIGITAL_INPUT_STATUS:
            break;
        case ID_MC_MOTOR_POSITION_INFORMATION:
        {
            CAN_message_mc_motor_position_information_t *data =
                &msg->contents.mc_motor_position_information;
            printf("MOTOR ANGLE: %f\n"
                   "MOTOR SPEED: %hd RPM\n"
                   "ELEC OUTPUT FREQ: %f\n"
                   "DELTA RESOLVER FILT: %hd\n",
                   data->motor_angle / 10.,
                   data->motor_speed,
                   data->electrical_output_frequency / 10.,
                   data->delta_resolver_filtered);
            current_status.mc_motor_position_information = *data;
            break;
        }
        case ID_MC_CURRENT_INFORMATION:
        {
            CAN_message_mc_current_information_t *data = &msg->contents.mc_current_information;
            printf("PHASE A CURRENT: %f A\n"
                   "PHASE B CURRENT: %f A\n" 
                   "PHASE C CURRENT: %f A\n"
                   "DC BUS CURRENT: %f A\n",
                   data->phase_a_current / 10.,
                   data->phase_b_current / 10.,
                   data->phase_c_current / 10.,
                   data->dc_bus_current / 10.);
            current_status.mc_current_information = *data;
            break;
        }
        case ID_MC_VOLTAGE_INFORMATION:
        {
            CAN_message_mc_voltage_information_t *data = &msg->contents.mc_voltage_information;
            printf("DC BUS VOLTAGE: %f V\n"
                   "OUTPUT VOLTAGE: %f V\n"
                   "PHASE AB VOLTAGE: %f V\n"
                   "PHASE BC VOLTAGE: %f V\n",
                   data->dc_bus_voltage / 10.,
                   data->output_voltage / 10.,
                   data->phase_ab_voltage / 10.,
                   data->phase_bc_voltage / 10.);
            current_status.mc_voltage_information = *data;
            break;
        }
        case ID_MC_FLUX_INFORMATION:
            break;
        case ID_MC_INTERNAL_VOLTAGES:
            break;
        case ID_MC_INTERNAL_STATES:
        {
            CAN_message_mc_internal_states_t *data = &msg->contents.mc_internal_states;
            printf("VSM STATE: %hu\n"
                   "INVERTER STATE: %hhu\n"
                   "INVERTER RUN MODE: %hhu\n"
                   "INVERTER ACTIVE DISCHARGE STATE: %hhu\n"
                   "INVERTER COMMAND MODE: %hhu\n"
                   "INVERTER ENABLE: %hhu\n"
                   "INVERTER LOCKOUT: %hhu\n"
                   "DIRECTION COMMAND: %hhu\n",
                   data->vsm_state,
                   data->inverter_state,
                   (char)(data->inverter_run_mode_discharge_state & 1),
                   (char)((data->inverter_run_mode_discharge_state & 0xE0) >> 5),
                   data->inverter_command_mode,
                   (char)(data->inverter_enable & 1),
                   (char)((data->inverter_enable & 0x80) >> 7),
                   data->direction_command);
            current_status.mc_internal_states = *data;
        }
        case ID_MC_FAULT_CODES:
        {
            CAN_message_mc_fault_codes_t *data = &msg->contents.mc_fault_codes;
            printf("POST FAULT LO: 0x%hX\n"
                   "POST FAULT HI: 0x%hX\n"
                   "RUN FAULT LO: 0x%hX\n"
                   "RUN FAULT HI: 0x%hx\n",
                   data->post_fault_lo,
                   data->post_fault_hi,
                   data->run_fault_lo,
                   data->run_fault_hi);
            current_status.mc_fault_codes = *data;
            break;
        }
        case ID_MC_TORQUE_TIMER_INFORMATION:
        {
            CAN_message_mc_torque_timer_information_t *data =
                &msg->contents.mc_torque_timer_information;
            printf("COMMANDED TORQUE: %f Nm\n"
                   "TORQUE FEEDBACK: %f Nm\n"
                   "RMS UPTIME: %f s",
                   data->commanded_torque / 10.,
                   data->torque_feedback / 10.,
                   data->power_on_timer * 0.003);
            current_status.mc_torque_timer_information = *data;
            break;
        }
        case ID_MC_MODULATION_INDEX_FLUX_WEAKENING_OUTPUT_INFORMATION:
            break;
        case ID_MC_FIRMWARE_INFORMATION:
            break;
        case ID_MC_DIAGNOSTIC_DATA:
            break;
        case ID_MC_COMMAND_MESSAGE: {
            CAN_message_mc_command_message_t *data = &msg->contents.mc_command_message;
            // TODO add the other members of this struct??
            printf("FCU REQUESTED TORQUE: %f\n",
                   data->torque_command / 10.);
            current_status.mc_command_message = *data;
        }
        case ID_MC_READ_WRITE_PARAMETER_COMMAND:
            break;
        case ID_MC_READ_WRITE_PARAMETER_RESPONSE:
            break;
        default:
    }
}




static void connection_lost(void *context, char *cause)
{
    printf("Connection lost: %s\n", cause);
}

static void msg_delivered(void *context, char *topic_name, int topic_len,
        MQTTClient_message *msg)
{
    // Do nothing for now.
}

static void msg_arrived(void *context, char *topic_name, int topic_len,
        MQTTClient_message *msg)
{
    char *payload_str = msg->payload;
    int payload_len = msg->payloadlen;
    // TODO cobs
    for (int i = 0; payload_str[i] != 0; i++) {
        if (payload_str[i] == ',') {
            payload_str[i] = 0;
            uint64_t timestamp = atoll(payload_str);
            payload = (CAN_message_t *)&payload_str[i + 1];
            process_message(timestamp, payload);
        }
    }
    fprintf(stderr, "Message formatted improperly\n");
}

int main(int argc, char* argv[])
{
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

	MQTTClient_create(&client, ADDRESS, CLIENTID,
			MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connection_lost, msg_arrived,
            msg_delivered);

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", rc);
		exit(-1);
	}
	pubmsg.payload = PAYLOAD;
	pubmsg.payloadlen = strlen(PAYLOAD);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
	printf("Waiting for up to %d seconds for publication of %s\n"
			"on topic %s for client with ClientID: %s\n",
			(int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	printf("Message with delivery token %d delivered\n", token);
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	return rc;
}