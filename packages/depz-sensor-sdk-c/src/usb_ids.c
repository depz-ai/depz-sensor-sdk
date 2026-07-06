/* DEPZ USB identity table (contract 02 §4.2).
 *
 * A single, one-edit-to-update table. Discovery uses (vid,pid) only to pick
 * candidate serial ports cheaply; the firmware-name probe is authoritative.
 * The PID->model map is an informational hint only.
 */
#include "depz_sensor_sdk.h"

/* PID -> sensor-model hint (informational). Full official DEPZ PID catalog
 * (VID 0x1BCF). One-edit-to-update: keep in sync with contracts/vectors/usb_ids.json. */
static const struct { int pid; const char *model; } DEPZ_PID_MODEL[] = {
    { DEPZ_PID_SR04,   "sr04" },     /* 0xEC78 60536 */
    { DEPZ_PID_VL53L8, "vl53l8ch" }, /* 0xED40 60736 */
    { 0xED41,          "vl53l0x" },  /* 60737 */
    { 0xED42,          "vl53l1cb" }, /* 60738 */
    { 0xED43,          "vl53l1cx" }, /* 60739 */
    { 0xED44,          "vl53l3cx" }, /* 60740 */
    { 0xED45,          "vl53l4cd" }, /* 60741 */
    { 0xED46,          "vl53l4cx" }, /* 60742 */
    { 0xED47,          "vl53l4ed" }, /* 60743 */
    { 0xED48,          "vl53l5cx" }, /* 60744 */
    { 0xED49,          "vl53l7cx" }, /* 60745 */
    { 0xED4A,          "vl53l7ch" }, /* 60746 */
    { 0xED4B,          "vl53l8cx" }, /* 60747 — VL53L8CX production PID, hw-verified */
    { DEPZ_PID_BNO086, "bno086" },   /* 0xEE08 60936 */
    { 0xEE09,          "bno085" },   /* 60937 */
    { 0xEE0A,          "bno055" },   /* 60938 */
};
static const size_t DEPZ_PID_MODEL_N =
    sizeof(DEPZ_PID_MODEL) / sizeof(DEPZ_PID_MODEL[0]);

bool depz_is_known_depz_usb(int vid, int pid)
{
    if (vid < 0 || pid < 0)
        return false;
    if (vid == DEPZ_DEV_USB_VID && pid == DEPZ_DEV_USB_PID)
        return true;
    if (vid != DEPZ_USB_VID)
        return false;
    for (size_t i = 0; i < DEPZ_PID_MODEL_N; i++)
        if (DEPZ_PID_MODEL[i].pid == pid)
            return true;
    return pid >= (int)DEPZ_PID_RANGE_LO && pid <= (int)DEPZ_PID_RANGE_HI;
}

const char *depz_usb_model_hint(int vid, int pid)
{
    if (vid < 0 || pid < 0)
        return NULL;
    if (vid == DEPZ_DEV_USB_VID && pid == DEPZ_DEV_USB_PID)
        return "dev";
    if (vid == DEPZ_USB_VID) {
        for (size_t i = 0; i < DEPZ_PID_MODEL_N; i++)
            if (DEPZ_PID_MODEL[i].pid == pid)
                return DEPZ_PID_MODEL[i].model;
    }
    return NULL;
}
