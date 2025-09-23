/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include<interrupts.hpp>

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
int isr_ms = 40; // isr body time per invocation, vary for experiments


// completion times per device id. sized from delays vector length
std::vector<int> io_done(delays.size(), -1);


// helper. trim whitespace from activity tokens
auto trim = [](std::string s){
while(!s.empty() && (s.front()==' ' || s.front()=='\t')) s.erase(s.begin());
while(!s.empty() && (s.back()==' ' || s.back()=='\t' || s.back()=='\r')) s.pop_back();
return s;
};


// helper. append one line to execution
auto append = [&](int start, int dur, const std::string& event){
execution += std::to_string(start) + ", " + std::to_string(dur) + ", " + event + "\n";
};


// helper. return from kernel to user
auto run_return_to_user = [&](){
append(now_ms, ctx_ms, "context restored");
now_ms += ctx_ms;
append(now_ms, 1, "switch to user mode");
now_ms += 1;
};


    /******************************************************************/

    //parse each line of the input trace file
    while(std::getline(input_file, trace)) {
        auto [activity, duration_intr] = parse_trace(trace);

        /******************ADD YOUR SIMULATION CODE HERE*************************/

        activity = trim(activity);


        if(activity == "CPU" || activity == "cpu") {
        // cpu burst. advance time by given duration
        append(now_ms, duration_intr, "cpu");
        now_ms += duration_intr;
        }
        else if(activity == "SYSCALL" || activity == "syscall") {
        // begin i o using device id = duration_intr
        int dev = duration_intr;


        // enter kernel and locate isr
        auto [pre, t_after] = intr_boilerplate(now_ms, dev, ctx_ms, vectors);
        execution += pre;
        now_ms = t_after;


        // execute the isr body for the request
        append(now_ms, isr_ms, "execute isr body");
        now_ms += isr_ms;


        // schedule device completion at current time plus device delay
        if(dev >= 0 && dev < (int)delays.size()) io_done[dev] = now_ms + delays[dev];


        // return to user
        append(now_ms, 1, "IRET");
        now_ms += 1;
        run_return_to_user();
        }
        else if(activity == "END_IO" || activity == "end_io" || activity == "END-IO" || activity == "END IO") {
            int dev = duration_intr; //dev is 7

            int t_done = (dev >= 0 && dev < (int)io_done.size()) ? io_done[dev] : now_ms;

            if(now_ms < t_done) now_ms = t_done;

            append(now_ms,delays[dev],"end of i/o " + std::to_string(dev) + ": interrupt");

            auto[pre, t_after] = intr_boilerplate(now_ms, dev, ctx_ms, vectors);
            execution += pre;
            now_ms = t_after;

            append(now_ms, isr_ms, "execute isr body");
            now_ms += isr_ms;


            // schedule device completion at current time plus device delay
            if(dev >= 0 && dev < (int)delays.size()) io_done[dev] = now_ms + delays[dev];


            // return to user
            append(now_ms, 1, "IRET");
            now_ms += 1;
            run_return_to_user();







        




        }
        else {
        // unknown activity. ignore or report
        append(now_ms, 0, std::string("unknown activity ") + activity);

        
        }
        append(0,0,std::string("finished line"));

        /************************************************************************/

    }

    input_file.close();

    write_output(execution);

    return 0;
}
