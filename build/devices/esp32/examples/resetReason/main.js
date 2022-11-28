import reset_reason from "resetReason";
import reset from "reset";
import Timer from "timer";

trace(`Reset reason: ${reset_reason().string}\n`);

Timer.set(() => {
    trace("Resetting in software.\n");
    reset();
}, 5000);