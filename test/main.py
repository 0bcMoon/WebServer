
import lldb

# Initialize the debugger
debugger = lldb.SBDebugger.Create()
debugger.SetAsync(False)

# Create a target from your compiled binary (replace with your binary file)
target = debugger.CreateTargetWithFileAndArch("main.cpp", lldb.LLDB_ARCH_DEFAULT)

# Set a breakpoint at line 39 in main.cpp
breakpoint = target.BreakpointCreateByLocation("main.cpp", 39)

# Launch the process
process = target.LaunchSimple(None, None, ".")

# Check if the process was successfully launched
if process:
    # Fetch the current thread and frame
    thread = process.GetSelectedThread()
    frame = thread.GetFrameAtIndex(0)

    # Evaluate and print the values of the variables
    global_pram = frame.EvaluateExpression("http->globalPram")
    servers = frame.EvaluateExpression("servers")

    print("http->globalPram:", global_pram.GetValue())
    print("servers:", servers.GetValue())
else:
    print("Failed to launch process")
