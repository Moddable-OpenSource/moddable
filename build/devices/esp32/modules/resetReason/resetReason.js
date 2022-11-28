const reasons = [
    "Unknown",
    "Power-On",
    "External Pin",
    "Software",
    "Panic",
    "Interrupt WDT",
    "Task WDT",
    "Other WDT",
    "Deep Sleep",
    "Brownout",
    "SDIO"
]

function _reset_reason() @ "xs_esp32_reset_reason";

export default function getResetReason() {
    const reason = _reset_reason();
    let string = "Invalid";

    if (reason >= 0 && reason < reasons.length)
        string = reasons[reason];
        
    return {reason,string};
}