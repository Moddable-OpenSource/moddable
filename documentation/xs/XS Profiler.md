# XS Profiler
Updated: December 1, 2022

The XS profiler is a sample based JavaScript profiler. The XS profiler also reports time spent in native functions or in the garbage collector. This document is a guide to the implementation of the profiler in XS. The information it contains, particularly in the [Viewers](#viewers) section, is useful in interpreting the results of a profiling session.

There are two implementations of the profiler:

- **Instrument**: XS sends profile records and samples to **xsbug**. The instrument profiler is used by Moddable SDK applications in the simulator and on devices. 

> Build XS with `mxInstrument` defined.

- **File**: XS accumulates profile records and samples in RAM then saves them into a [`.cpuprofile`](https://chromedevtools.github.io/devtools-protocol/tot/Profiler/#type-Profile) file. The file profiler is used by command line tools including **xst** and **xsnap**. 

> Build XS with `mxProfile` defined and xsProfile.c included. 

Both profilers require debugging information, so scripts must be built with debugging enabled. The instrument profiler requires a connection to **xsbug**. Therefore, profiling is only supported in builds of XS with debugging support enabled.

<a id="profile"></a>
## Profile
This section describes the data the XS profiler collects and how it transmits and stores that data.

### ID

Every function has a unique profile ID, which is assigned when the function is defined. All function instances created from the same function definition have the same profile ID. For example, the following creates only a single profile ID:

```js
for (let i = 0; i < 10; i++) {
	const t = function () {
		trace(i);
	};
	t();
}
```

For function instances, the profile ID is stored in the ID of the internal home slot. For function primitives, the profile ID is stored in the slot itself. This approach means that profile IDs require no additional RAM or storage space. All debug builds are ready to be profiled.
 
The profile ID of the host is 0. The profile ID of the garbage collector is 1. For newly created machines, function profile IDs start incrementing from 2; for cloned machines, from the profile ID stored in the preparation.

### Record

A profile record is created for each function observed to run during a profile session. The profile record contains:

- Profile ID
- Function identifier (name)
- Path (source file)
- Line number

Profile records are only stored once by the file profiler and only sent once to **xsbug** by the instrument profiler.

The home slot allows function identifiers to be more detailed for function instances (`Array.prototype.push`) than for function primitives (`push`). 

### Sample

A profile sample is created for each sample taken during a profile session. The profile sample contains:

- Time delta: microseconds since the last sample.
- Profile ID stack: sequence of profile ID in stack order. The first profile ID is the function hit by the profiler. The last profile ID is always the host (0).

For each sample, The instrument profiler simply  sends the sample to **xsbug**, which updates the call graph and the durations of the profile records. The file profiler updates the call graph and stores the time delta and the first profile ID. 

### Time

XS checks the profiler to determine if a sample should be taken at several points in execution:

- Each `LINE` byte code
- Exiting a native function
- Exiting the garbage collector
- Entering the machine from the host

<a id="memoryandperformance"></a>
## Memory and Performance

Starting and stopping the profiler creates and deletes a profiler. No RAM is used when the profiler is inactive.

Both the instrument and file profilers allocate their buffers outside the XS heaps. These ensures that enabling profiling does not change the runtime behavior of the garbage collector.

### Instrument Profiler

The instrument profiler uses:

- A bitmap to remember which profile records have already been sent to **xsbug**, i.e. one bit for each profile ID. 
- Buffers for profile samples. The size of the buffers is the size of the XS stack divided by the size of a frame and multiplied by the size of a profile ID.

### File Profiler

The file profiler uses growing buffers to store profile records and samples. 

<a id="howtouse"></a>
## How To Use

### Instrument Profiler

In xsbug, the stopwatch icon starts and stops the profiler. The icon is located at the top-right of the PROFILE pane of a debuggee tab.

There is also a preference to automatically start the profiler when the debuggee starts.

### File Profiler

For **xst**, the `-p` option starts and stops the profiler around the execution of the script or module. The `.cpuprofile` file containing the profile results is saved in the current directory.

The `.cpuprofile` file can be viewed in **Google Chrome DevTools**, **Visual Studio Code**, and **xsbug**.

### Programming Interface

Use the `xsStartProfiling` and `xsStopProfiling` macros to start and stop the profiler.

<a id="viewers"></a>
## Viewers

Because you can open the `.cpuprofile` file created by the File Profiler with at least three different applications, you get somewhat different views of the same data.

In the three viewers, the first column is the "Self Time", the second column is the "Total Time". 

Google Chrome DevTools and xsbug can toggle between a "Bottom Up" viewer (from callees to callers) and a "Top Down" viewer (from callers to callees).

For reference, here are links to relevant source files:

- Google Chrome DevTools: [CPUProfileDataModel.ts](https://github.com/ChromeDevTools/devtools-frontend/blob/main/front_end/core/sdk/CPUProfileDataModel.ts)
- Visual Studio Code: [profilingModel.ts](https://github.com/microsoft/vscode/blob/main/src/vs/platform/profiling/common/profilingModel.ts)
- xsbug: [ProfilePane.js](https://github.com/Moddable-OpenSource/moddable/blob/public/tools/xsbug/DebugPane.js)

### Self Time

The Self Time is the sum of the sample durations of functions hit by the profiler, i.e. functions that were executing when the profiler sampled. The sample duration is the time delta between a sample and its next sample.

Visual Studio Code and xsbug display the same  Self Time. Google Chrome DevTools does not, although values are similar.

### Total Time

Visual Studio Code computes the Total Time of a function by recursively  adding, from callers to callees, the Self Time and the Total Time of all its callees. Since one function can be called by several different functions, the results can be greater than the duration of the profile itself.

xsbug computes the Total Time by recursively propagating the Self Time from callees to callers, dividing the propagated duration by the number of callers at each step. No results are greater than the duration of the profile itself.

Visual Studio Code and xsbug agree on the Total Time if the call graph is a tree.

How Google Chrome DevTools compute the Total Time? To be investigated but the results are closer to xsbug than to Visual Studio Code.

### Notes

- To be able to open a `.cpuprofile` file in Visual Studio Code, there cannot be cycles in the call graph. Therefore, the File Profiler eliminates cycles when saving the file.
- Both Google Chrome DevTools and Visual Studio Code merge profile records by location (function name, file, line). xsbug does not since the XS engine ensures that there is only one profile record for each function.
- Google Chrome DevTools crashes when opening `.cpuprofile` files saved by **xsnap** when replaying a session of the Agoric runtime.
