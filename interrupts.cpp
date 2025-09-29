/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include <interrupts.hpp>

int main(int argc, char **argv)
{

    // vectors is a C++ std::vector of strings that contain the address of the ISR
    // delays  is a C++ std::vector of ints that contain the delays of each device
    // the index of these elemens is the device number, starting from 0
    auto [vectors, delays] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    std::string trace;     //!< string to store single line of trace file
    std::string execution; //!< string to accumulate the execution output
                           /******************ADD YOUR VARIABLES HERE*************************/
    int current_time = 0;

    // Tunables (keep simple; tweak if asked)
    int context_save_ms = 2;     // time to save context on interrupt/trap
    int return_overhead_ms = 1;  // “return to user” / IRET bookkeeping
    int syscall_overhead_ms = 1; // light work to arm/start I/O

    // Optional: bounds check helpers
    auto check_dev = [&](int dev)
    {
        if (dev < 0 || dev >= (int)delays.size() || dev >= (int)vectors.size())
        {
            std::cerr << "Invalid device/vector id: " << dev << std::endl;
            exit(1);
        }
    };

    // Emit one timeline row
    auto emit = [&](int start, int dur, const std::string &what)
    {
        execution += std::to_string(start) + ", " + std::to_string(dur) + ", " + what + "\n";
    };
    /******************************************************************/

    // parse each line of the input trace file
    while (std::getline(input_file, trace))
    {
        auto [activity, duration_intr] = parse_trace(trace);

        /******************ADD YOUR SIMULATION CODE HERE*************************/
        if (activity == "CPU")
        {
            int ms = duration_intr;
            emit(current_time, ms, "user CPU burst");
            current_time += ms;
        }
        else if (activity == "SYSCALL") {
    int dev = duration_intr;
    check_dev(dev);

    // Hardware entry sequence
    auto [prelog, t_after] = intr_boilerplate(current_time, dev, context_save_ms, vectors);
    execution += prelog;
    current_time = t_after;

    // Syscall work: start I/O
    emit(current_time, syscall_overhead_ms, "syscall: start I/O on device " + std::to_string(dev));
    current_time += syscall_overhead_ms;

    // Process goes to waiting state
    emit(current_time, 1, "process moved to waiting state");
    current_time++;

    // Scheduler picks another process
    emit(current_time, scheduler_overhead_ms, "scheduler selects next process");
    current_time += scheduler_overhead_ms;

    // Return to user mode
    emit(current_time, return_overhead_ms, "return to user mode");
    current_time += return_overhead_ms;
}

       else if (activity == "END_IO") {
    int dev = duration_intr;
    check_dev(dev);

    // Hardware entry sequence
    auto [prelog, t_after] = intr_boilerplate(current_time, dev, context_save_ms, vectors);
    execution += prelog;
    current_time = t_after;

    // Device-specific ISR body
    int isr_ms = delays.at(dev);
    emit(current_time, isr_ms, "run ISR for device " + std::to_string(dev));
    current_time += isr_ms;

    // Clear interrupt / acknowledge
    emit(current_time, 1, "EOI/clear device flag");
    current_time++;

    // Process moves to ready state
    emit(current_time, 1, "process moved to ready state");
    current_time++;

    // Scheduler dispatches process
    emit(current_time, scheduler_overhead_ms, "scheduler dispatches process");
    current_time += scheduler_overhead_ms;

    // Return from interrupt
    emit(current_time, return_overhead_ms, "return from interrupt");
    current_time += return_overhead_ms;
}
        else {
            std::cerr << "Unknown activity: " << activity << std::endl;
            exit(1);
        }
        /************************************************************************/
        
    }

    input_file.close();

    write_output(execution);

    return 0;
}
