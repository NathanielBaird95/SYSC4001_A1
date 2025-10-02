/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include <interrupts.hpp>

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    auto [vectors, delays] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    std::string trace;      //!< string to store single line of trace file
    std::string execution;  //!< string to accumulate the execution output

    /******************ADD YOUR VARIABLES HERE*************************/
    int now_ms = 0; // simulation clock in ms
    int ctx_ms = 10; // context save or restore time, vary for experiments
    int isr_ms = 190; // isr body time per invocation, vary for experiments

    // completion times per device id. sized from delays vector length
    std::vector<int> io_done(delays.size(), -1);

    // return from kernel to user
    auto run_return_to_user = [&](){
        execution += std::to_string(now_ms) + ", " + std::to_string(ctx_ms) + ", context restored\n";
        now_ms += ctx_ms;
        execution += std::to_string(now_ms) + ", 1, switch to user mode\n";
        now_ms += 1;
    };
    /******************************************************************/

    //parse each line of the input trace file
    while (std::getline(input_file, trace)) {
        auto [activity, duration_intr] = parse_trace(trace);

        /******************ADD YOUR SIMULATION CODE HERE*************************/

        // inline trim of activity
        while (!activity.empty() && (activity.front() == ' ' || activity.front() == '\t')) activity.erase(activity.begin());
        while (!activity.empty() && (activity.back() == ' ' || activity.back() == '\t' || activity.back() == '\r')) activity.pop_back();

        if (activity == "CPU" || activity == "cpu") {
            // cpu burst. advance time by given duration
            execution += std::to_string(now_ms) + ", " + std::to_string(duration_intr) + ", cpu\n";
            now_ms += duration_intr;
        }
        else if (activity == "SYSCALL" || activity == "syscall") {
            // begin i o using device id = duration_intr
            int dev = duration_intr;

            // enter kernel and locate isr
            auto [pre, t_after] = intr_boilerplate(now_ms, dev, ctx_ms, vectors);
            execution += pre;
            now_ms = t_after;

            // execute the isr body for the request
            execution += std::to_string(now_ms) + ", " + std::to_string(isr_ms) + ", execute isr body\n";
            now_ms += isr_ms;

            // schedule device completion at current time plus device delay
            if (dev >= 0 && dev < static_cast<int>(delays.size())) io_done[dev] = now_ms + delays[dev];

            // return to user
            execution += std::to_string(now_ms) + ", 1, IRET\n";
            now_ms += 1;
            run_return_to_user();
        }
        else if (activity == "END_IO" || activity == "end_io" || activity == "END-IO" || activity == "END IO") {
            int dev = duration_intr; // dev is from trace

            execution += std::to_string(now_ms) + ", " + std::to_string(delays[dev]) + ", end of i/o " + std::to_string(dev) + ": interrupt\n";
            now_ms += delays[dev];

            auto [pre, t_after] = intr_boilerplate(now_ms, dev, ctx_ms, vectors);
            execution += pre;
            now_ms = t_after;

            execution += std::to_string(now_ms) + ", " + std::to_string(isr_ms) + ", execute isr body\n";
            now_ms += isr_ms;

            if (dev >= 0 && dev < static_cast<int>(delays.size())) io_done[dev] = now_ms + delays[dev];

            execution += std::to_string(now_ms) + ", 1, IRET\n";
            now_ms += 1;
            run_return_to_user();
        }
        else {
            // unknown activity. ignore or report
            execution += std::to_string(now_ms) + ", 0, unknown activity " + activity + "\n";
        }

        /************************************************************************/
    }

    input_file.close();

    write_output(execution);

    return 0;
}
