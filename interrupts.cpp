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

        /************************************************************************/
    }

    input_file.close();

    write_output(execution);

    return 0;
}