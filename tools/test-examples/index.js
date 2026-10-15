/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
    Generated with Google Gemini 3.1 Pro using Google Antigravity. Beware.
*/

const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const args = process.argv.slice(2);
const target = args[0];

if (!target || target === '--help' || target === '-h') {
    const isHelp = target === '--help' || target === '-h';
    console[isHelp ? 'log' : 'error'](`
Moddable SDK Automated Test Harness
===================================
Builds, deploys, and verifies Moddable SDK examples.
Uses xsdb verify exception free launch.

Examples with a 'package.json' are built using 'mcpack mcconfig'.
If 'package.json' declares a 'build' script (e.g. TypeScript projects),
'npm run build' is invoked first, and 'npm install' is run when
'node_modules' is missing.

Usage: node index.js <target> [options]

Arguments:
  <target>                Platform target to build for (e.g., mac, sim/moddable_six, esp32/moddable_six).
                          The subplatform may use a wildcard (e.g., esp32/*, esp32/m5*) to build
                          against every matching target. A wildcard implies '--mode build', since
                          only one device can be connected at a time.

Options:
  --dir <path>            Run tests recursively from a directory. Defaults to $MODDABLE/examples.
  --example <path>        Run a single example at the given path.
  --continue, -c          Resume interrupted test run from report.json
  --mode <mode>           Execution mode. Use '--mode build' to skip device deployment and only test build.
  --clean                 Always perform a clean build.
  --ssid <ssid>           Wi-Fi SSID for testing networking examples on embedded hardware.
  --password <password>   Wi-Fi Password for SSID.
  --help, -h              Display this help.

Output:
  'report.json' is output to tools/test-examples/ on completion. It opens with
  the settings and time of the run, followed by one entry per example. Failures
  include the full build log; successes omit it to keep the report small.
`);
    process.exit(isHelp ? 0 : 1);
}

let ssid = '';
let password = '';
let specificExample = '';
let customDir = '';
let isContinue = false;
let mode = '';
let cleanBuild = false;
let deviceInfo = null;

for (let i = 1; i < args.length; i++) {
    if (args[i] === '--ssid') {
        ssid = args[++i];
    } else if (args[i].startsWith('--ssid=')) {
        ssid = args[i].substring(7);
    } else if (args[i] === '--password') {
        password = args[++i];
    } else if (args[i].startsWith('--password=')) {
        password = args[i].substring(11);
    } else if (args[i] === '--example') {
        specificExample = args[++i];
    } else if (args[i].startsWith('--example=')) {
        specificExample = args[i].substring(10);
    } else if (args[i] === '--dir') {
        customDir = args[++i];
    } else if (args[i].startsWith('--dir=')) {
        customDir = args[i].substring(6);
    } else if (args[i] === '--mode') {
        mode = args[++i];
    } else if (args[i].startsWith('--mode=')) {
        mode = args[i].substring(7);
    } else if (args[i] === '--clean') {
        cleanBuild = true;
    } else if (args[i] === '--continue' || args[i] === '-c') {
        isContinue = true;
    }
}

const moddableDir = process.env.MODDABLE;
if (!moddableDir) {
    console.error("Please set MODDABLE environment variable.");
    process.exit(1);
}

// Ensure the user's PATH points to a release build of the host tools to prevent xsdb halts on exceptions
try {
    const cmdStr = process.platform === 'win32' ? 'where mcconfig' : 'which mcconfig';
    const mcconfigPath = require('child_process').execSync(cmdStr, { encoding: 'utf-8' }).trim();
    if (mcconfigPath.toLowerCase().includes('debug')) {
        console.error("Error: The test harness requires the RELEASE host tools to avoid host-side debugger interruptions.");
        console.error(`Currently using debug version at: ${mcconfigPath}`);
        console.error("Please update your PATH environment variable to point to the release tools directory.");
        process.exit(1);
    }
} catch {
    console.error("Error: 'mcconfig' not found in PATH.");
    process.exit(1);
}

// Validate paths up front, so a typo is one clear error rather than a failure per example and target.
function checkDirectory(option, value) {
    if (!value) {
        console.error(`Error: ${option} requires a path.`);
        process.exit(1);
    }
    const resolved = path.resolve(value);
    let stat;
    try {
        stat = fs.statSync(resolved);
    } catch {
        console.error(`Error: ${option} directory doesn't exist: ${resolved}`);
        process.exit(1);
    }
    if (!stat.isDirectory()) {
        console.error(`Error: ${option} path is not a directory: ${resolved}`);
        process.exit(1);
    }
    return resolved;
}

if (args.some(a => a === '--example' || a.startsWith('--example='))) {
    specificExample = checkDirectory('--example', specificExample);
    if (!fs.existsSync(path.join(specificExample, 'manifest.json')) && !fs.existsSync(path.join(specificExample, 'package.json'))) {
        console.error(`Error: --example directory has no manifest.json or package.json: ${specificExample}`);
        process.exit(1);
    }
}
if (args.some(a => a === '--dir' || a.startsWith('--dir=')))
    customDir = checkDirectory('--dir', customDir);

const examplesDir = customDir ? path.resolve(customDir) : path.join(moddableDir, 'examples');

// A wildcard in the subplatform (e.g. esp32/m5*) expands to every matching target
// directory. Only one device can be connected, so wildcard runs are build-only.
let targets = [target];
if (target.includes('*')) {
    const slash = target.indexOf('/');
    const platform = (slash >= 0) ? target.slice(0, slash) : target;
    const pattern = (slash >= 0) ? target.slice(slash + 1) : '';
    if (platform.includes('*') || !pattern) {
        console.error("Error: a wildcard is only supported in the subplatform (e.g. esp32/*, esp32/m5*). The platform must be given.");
        process.exit(1);
    }

    const targetsDir = (platform === 'sim')
        ? path.join(moddableDir, 'build', 'simulators')
        : path.join(moddableDir, 'build', 'devices', platform, 'targets');
    let names = [];
    try {
        names = fs.readdirSync(targetsDir, { withFileTypes: true })
            .filter(entry => entry.isDirectory() && (entry.name !== 'modules'))
            .map(entry => entry.name);
    } catch {
        console.error(`Error: platform '${platform}' has no targets directory (${targetsDir}).`);
        process.exit(1);
    }

    const regex = new RegExp('^' + pattern.split('*').map(part => part.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')).join('.*') + '$');
    targets = names.filter(name => regex.test(name)).sort().map(name => `${platform}/${name}`);
    if (!targets.length) {
        console.error(`Error: no targets under ${targetsDir} match '${pattern}'.`);
        process.exit(1);
    }

    if (mode && mode !== 'build')
        console.warn(`[Test Harness] Wildcard target: ignoring '--mode ${mode}'; builds only.`);
    mode = 'build';
    console.log(`[Test Harness] Target '${target}' matches ${targets.length} target${(targets.length === 1) ? '' : 's'}: ${targets.map(t => t.slice(slash + 1)).join(', ')}`);
}

if (target.startsWith('esp32/')) {
    const idfPath = process.env.IDF_PATH;
    if (!idfPath) {
        console.error("Please set IDF_PATH environment variable for ESP32 targets.");
        process.exit(1);
    }
    
    // Check if IDF is already properly sourced in the environment
    try {
        require('child_process').execSync(process.platform === 'win32' ? 'where idf.py' : 'which idf.py', { stdio: 'ignore' });
    } catch {
        process.stdout.write("Sourcing ESP-IDF environment globally... ");
        try {
            let cmd;
            if (process.platform === 'win32') {
                cmd = `set IDF_EXPORT_QUIET=1 && pushd "%IDF_PATH%" && "%IDF_TOOLS_PATH%\\idf_cmd_init.bat" && popd && set`;
            } else {
                cmd = `export IDF_EXPORT_QUIET=1 && source "${idfPath}/export.sh" > /dev/null 2>&1 && env`;
            }
            
            const envOutput = require('child_process').execSync(cmd, { shell: process.platform === 'win32' ? 'cmd.exe' : '/bin/bash', encoding: 'utf-8' });
            
            envOutput.split('\n').forEach(line => {
                const match = line.match(/^([^=]+)=(.*)$/);
                if (match) {
                    process.env[match[1]] = match[2].trim();
                }
            });
            console.log("Done.");
        } catch (err) {
            console.error("\nFailed to source ESP-IDF export script:", err.message);
            process.exit(1);
        }
    }
    
    // Probe device hardware configuration to avoid auto-probing delays during build
    if (mode !== 'build') {
        try {
            process.stdout.write("Probing ESP32 hardware via esptool... ");
            const portArg = process.env.UPLOAD_PORT ? ` --port ${process.env.UPLOAD_PORT}` : '';
            const esptoolOut = require('child_process').execSync(`esptool${portArg} chip-id`, { encoding: 'utf-8', env: process.env, stdio: ['ignore', 'pipe', 'ignore'] });
            const portMatch = esptoolOut.match(/Serial port\s+([^\r\n:]+)/);
            const chipMatch = esptoolOut.match(/Chip is\s+(.+)/);
            const featuresMatch = esptoolOut.match(/Features:\s+(.+)/);
            
            if (portMatch) {
                process.env.UPLOAD_PORT = portMatch[1].trim();
            }
            
            deviceInfo = {
                chip: chipMatch ? chipMatch[1].trim() : undefined,
                features: featuresMatch ? featuresMatch[1].trim() : undefined,
                port: process.env.UPLOAD_PORT
            };

            console.log("Done.");
            if (chipMatch) console.log(`   Chip: ${chipMatch[1].trim()}`);
            if (featuresMatch) console.log(`   Features: ${featuresMatch[1].trim()}`);
            if (portMatch) console.log(`   Port: ${process.env.UPLOAD_PORT}`);
        } catch {
            const portHelp = process.env.UPLOAD_PORT ? ` on $UPLOAD_PORT (${process.env.UPLOAD_PORT})` : '';
            console.error(`\nError: Failed to probe ESP32 via esptool${portHelp}. Is the device connected?\nUse '--mode build' to test builds without a device.`);
            process.exit(1);
        }
    }
}

function findExamples(dir) {
    let results = [];
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
        if (entry.name.startsWith('.') || entry.name === 'node_modules') continue;
        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            const manifestPath = path.join(fullPath, 'manifest.json');
            const packageJsonPath = path.join(fullPath, 'package.json');
            if (fs.existsSync(manifestPath) || fs.existsSync(packageJsonPath)) {
                results.push(fullPath);
            } else {
                results = results.concat(findExamples(fullPath));
            }
        }
    }
    return results;
}

// ---------------------------------------------------------------------------
// Failure diagnosis
//
// The harness sees only what the tools print. These helpers turn that output
// into a single sentence naming what actually went wrong, so 'report.json' can
// be read without opening the attached log.
// ---------------------------------------------------------------------------

const ANSI = /\x1b\[[0-9;]*[A-Za-z]/g;

function clip(text, limit = 160) {
    const line = String(text).replace(ANSI, '').replace(/\s+/g, ' ').trim();
    return (line.length > limit) ? line.slice(0, limit - 1) + '…' : line;
}

// The tools end their error messages with '!'; drop it so it can be embedded in a sentence.
function trimBang(text) {
    return clip(text).replace(/!+$/, '');
}

function relativePath(p) {
    if (typeof p !== 'string' || !p) return '';
    const rel = path.relative(moddableDir, p);
    return (rel && !rel.startsWith('..')) ? rel : p;
}

// True when two target log events are close enough together to describe one failure.
function nearInTime(earlier, when) {
    if (!earlier) return false;
    if ((typeof earlier.time !== 'number') || (typeof when !== 'number')) return true;
    return (when - earlier.time) <= 1000;
}

// Formats the 'path' and 'line' carried by xsdb 'log' and 'stopped' events.
function describeLocation(where) {
    if (!where || !where.path) return '';
    const line = (typeof where.line === 'number' && where.line > 0) ? `:${where.line}` : '';
    return ` (at ${relativePath(where.path)}${line})`;
}

// Explains a non-zero exit from mcconfig/make. Each tool and toolchain in the
// build has its own error syntax; the most specific match wins.
function describeBuildFailure(output) {
    const lines = output.replace(/\r/g, '\n').replace(ANSI, '').split('\n');
    let platform = null, tool = null, compile = null, link = null, flash = null, generic = null, make = null;

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i].trim();
        let m;

        if (line === '### Error: incompatible platform!') {
            // The explanation is on the '# error:' line mcconfig printed just above.
            let why = '';
            for (let j = i - 1; (j >= 0) && (j >= i - 4); j--) {
                const above = lines[j].trim().match(/^# error: (.+)$/);
                if (above) {
                    why = `: ${trimBang(above[1])}`;
                    break;
                }
            }
            platform ??= `Example does not support this target${why}`;
        }
        else if ((m = line.match(/^A fatal error occurred: (.+)$/)))
            flash ??= `Flash failed: ${trimBang(m[1])}`;
        else if ((m = line.match(/^### ([A-Za-z][A-Za-z0-9]*Error: .+)$/)))
            tool ??= trimBang(m[1]);
        else if ((m = line.match(/^(.+?\.(?:c|cc|cpp|cxx|m|mm|h|hpp|S|js|ts)):(\d+)(?::\d+)?: (?:fatal )?error: (.+)$/)) ||
                 (m = line.match(/^(.+?\.(?:c|cc|cpp|cxx|m|mm|h|hpp|S|js|ts))\((\d+)(?:,\d+)?\): (?:fatal )?error:? (.+)$/)) ||
                 (m = line.match(/^(.+?)\((\d+),\d+\): error (TS\d+: .+)$/)))
            compile ??= `Compile error in ${path.basename(m[1])}:${m[2]}: ${trimBang(m[3])}`;
        else if ((m = line.match(/(section `[^`']+' will not fit in region `[^`']+')/)))
            link ??= `Link failed: ${m[1]} — the build is too large for this device`;
        else if (/^collect2: error: ld returned/.test(line))
            link ??= 'Link failed';
        else if ((m = line.match(/^# error: (.+)$/)))
            generic ??= trimBang(m[1]);
        else if ((m = line.match(/^make(?:\[\d+\])?: \*\*\* .*Error (\d+)/)))
            make ??= `Build failed (make error ${m[1]})`;
    }

    return platform || tool || compile || link || flash || generic || make || null;
}

// Last line worth quoting when nothing else identifies the failure.
function lastMeaningfulLine(output) {
    const lines = output.replace(/\r/g, '\n').replace(ANSI, '').split('\n');
    for (let i = lines.length - 1; i >= 0; i--) {
        const line = lines[i].replace(/\(xsdb\)\s*/g, '').trim();
        // "# " lines are build progress and "### -p" is the platform banner mcrun prints;
        // the remaining "###" lines are what the tools say when they fail.
        if (line && (line !== '{') && (line !== '}') && !/^# /.test(line) && !/^### -p /.test(line))
            return line;
    }
    return '';
}

// Distinguishes the ways a launch can time out. 'Did not launch', 'launched and
// crashed', and 'launched and hung' are three different bugs.
function describeRunTimeout(seconds, d) {
    const seen = d.lastMessage ? ` Last message from target: "${clip(d.lastMessage)}".` : '';

    if (!d.connected)
        return `Run timeout: target never connected to the debugger within ${seconds}s (it did not launch).`;

    if (d.restarts > 0)
        return `Run timeout: target restarted ${d.restarts} time${(d.restarts > 1) ? 's' : ''} within ${seconds}s (it launched, then crashed or rebooted).${seen}`;

    if (d.disconnected)
        return `Run timeout: target disconnected after ${d.samples} instrumentation sample${(d.samples === 1) ? '' : 's'} and did not reconnect within ${seconds}s (it launched, then died).${seen}`;

    if (d.waitingForIP)
        return `Run timeout: no IP address within ${seconds}s (the target is running — ${d.samples} instrumentation sample${(d.samples === 1) ? '' : 's'} received).${seen}`;

    if (!d.samples)
        return `Run timeout: target connected but sent no instrumentation within ${seconds}s (it launched, then stalled before the first sample).${seen}`;

    return `Run timeout: instrumentation stopped after ${d.samples} sample${(d.samples === 1) ? '' : 's'}, ${d.stalledForSeconds}s before the timeout (the application hung).${seen}`;
}

function runTest(examplePath, target) {
    return new Promise((resolve) => {
        const startTime = Date.now();
        const isEmbedded = !['mac', 'win', 'lin'].includes(target.toLowerCase()) && target.toLowerCase() !== 'sim' && !target.toLowerCase().startsWith('sim/');
        console.log(`\n========= Testing ${examplePath} (${target}) =========`);
        const manifestPath = path.join(examplePath, 'manifest.json');
        const packageJsonPath = path.join(examplePath, 'package.json');
        const isPackage = fs.existsSync(packageJsonPath);

        let manifestRaw = '';
        try {
            manifestRaw = fs.readFileSync(manifestPath, 'utf-8');
        } catch {}

        let packageJson = null;
        if (isPackage) {
            try {
                packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf-8'));
            } catch {}
        }

        // Package examples use mcpack, which always supplies the base manifest.
        if (!isPackage && !manifestRaw.includes('manifest_base.json')) {
            return resolve({ success: null, code: 'NO_BASE_MANIFEST', reason: "Skipped: manifest does not include 'manifest_base.json' (not an application)", log: '', durationMs: 0 });
        }

        let isNet = manifestRaw.includes('manifest_net.json');
        if (isPackage && !isNet) {
            // mcpack auto-includes networking when the source uses fetch / WebSocket globals.
            try {
                const mainRel = (packageJson && typeof packageJson.main === 'string') ? packageJson.main : './main.js';
                const mainPath = path.resolve(examplePath, mainRel);
                if (fs.existsSync(mainPath)) {
                    const mainSrc = fs.readFileSync(mainPath, 'utf-8');
                    if (/\bfetch\s*\(/.test(mainSrc) || /\bnew\s+WebSocket\b/.test(mainSrc)) {
                        isNet = true;
                    }
                }
            } catch {}
        }

        // mcpack invocation: 'mcconfig' for apps/hosts, 'mcrun' for mods.
        // Convention from examples/packages/readme.md: a mod's main entry is named 'mod.js'.
        const isMod = isPackage && packageJson && typeof packageJson.main === 'string' && path.basename(packageJson.main) === 'mod.js';
        const buildCmd = isPackage ? 'mcpack' : 'mcconfig';
        const buildPrefix = isPackage ? [isMod ? 'mcrun' : 'mcconfig'] : [];
        
        let mcArgs = ['-dl', '-m', '-p', target];
        let onlyBuild = false;
        
        if (mode === 'build') {
            mcArgs = ['-d', '-t', 'build', '-m', '-p', target];
            onlyBuild = true;
        }

        if (isNet && isEmbedded) {
            if (ssid && password && !onlyBuild) {
                mcArgs.push(`ssid=${ssid}`, `password=${password}`);
            } else if (!onlyBuild) {
                mcArgs = ['-d', '-t', 'build', '-m', '-p', target];
                onlyBuild = true;
            }
        }
        

        // xsdb reads .xsdb.json from $XSBUG_PROJECT, which the makefiles set to MAIN_DIR (the
        // manifest's directory). For mcconfig examples, MAIN_DIR is examplePath. For mcpack
        // examples, mcpack generates a manifest at $MODDABLE/build/tmp/<packageJson.name>/ and
        // passes that path to mcconfig — so MAIN_DIR points there instead.
        const xsdbConfigDirs = [examplePath];
        if (isPackage && packageJson && typeof packageJson.name === 'string') {
            const pkgParts = packageJson.name.split('/');
            xsdbConfigDirs.push(path.join(moddableDir, 'build', 'tmp', ...pkgParts));
        }

        const xsdbConfigSnapshots = xsdbConfigDirs.map(dir => {
            const p = path.join(dir, '.xsdb.json');
            let existed = false;
            let original = null;
            try {
                if (fs.existsSync(p)) {
                    existed = true;
                    original = fs.readFileSync(p, 'utf8');
                }
                fs.mkdirSync(dir, { recursive: true });
                fs.writeFileSync(p, JSON.stringify({ exceptionsMode: 'off', outputFormat: 'json' }));
            } catch {}
            return { path: p, existed, original };
        });

        const runMainBuild = () => {
            const child = spawn(buildCmd, [...buildPrefix, ...mcArgs], {
                cwd: examplePath,
                env: process.env,
                shell: process.platform === 'win32'
            });

        let outputBuf = '';
        let isSetup = false;
        
        let exceptionOccurred = false;
        // Tracking instrumentation state
        let ipDetected = !isNet || !isEmbedded;
        let instrumentPoller;
        let instrumentationDataCount = 0;
        let previousHistoryCount = -1;

        // Tracking what the target did, so a failure can say which of "never launched",
        // "launched and crashed", and "launched and hung" happened.
        let recentOutput = '';
        let connectCount = 0;
        let disconnectCount = 0;
        let sampleCount = 0;
        let lastSampleTime = 0;
        let lastException = null;
        let lastTargetMessage = null;
        
        let jsonBuffer = '';
        let openBraces = 0;
        let inJson = false;
        let inString = false;
        let escapeNext = false;
        
        function handleEvent(obj) {
            const data = obj.data || {};

            if (obj.event === 'stopped') {
                if (typeof data.reason === 'string') {
                    const reason = data.reason;
                    if (reason === '# Break: breakpoint!' || reason === '# Break: step!' || reason === '# Break: debugger!') {
                        // Resumes from deliberate programmatic breakpoints
                        child.stdin.write("c\n");
                    } else if (reason.startsWith('# Break:')) {
                        // Any other break is an unhandled exception or error (SyntaxError, TypeError, etc)
                        exceptionOccurred = true;
                        finish(false, 'Exception: ' + trimBang(reason.replace('# Break: ', '')) + describeLocation(data), 'EXCEPTION');
                    } else {
                        child.stdin.write("c\n");
                    }
                } else {
                    child.stdin.write("c\n");
                }
            } else if (obj.event === 'print') {
                // xsdb announces each connection and disconnection of the target here.
                const text = (typeof data.text === 'string') ? data.text : '';
                if (text.startsWith('Connected to '))
                    connectCount++;
                else if (text.includes('disconnected from'))
                    disconnectCount++;
            } else if (obj.event === 'log') {
                const text = (typeof data.text === 'string') ? data.text : '';
                const abort = text.indexOf('XS abort');
                if (abort >= 0) {
                    // The abort message names the failure; the '# Exception:' trace that
                    // precedes it names the file and line that raised it.
                    exceptionOccurred = true;
                    const where = nearInTime(lastException, data.time) ? lastException : null;
                    finish(false, trimBang(text.slice(abort)) + describeLocation(where), 'XS_ABORT');
                } else if (text.startsWith('# Exception:')) {
                    lastException = { path: data.path, line: data.line, time: data.time };
                } else if (text.trim()) {
                    lastTargetMessage = text;
                }
            } else if (obj.event === 'info_instruments') {
                if (typeof data.historyCount === 'number') {
                    if (data.historyCount > sampleCount) {
                        sampleCount = data.historyCount;
                        lastSampleTime = Date.now();
                    }
                    if (ipDetected && data.historyCount > previousHistoryCount) {
                        previousHistoryCount = data.historyCount;
                        instrumentationDataCount++;
                        if (instrumentationDataCount >= 3 && !exceptionOccurred) {
                            finish(true, 'Successful execution');
                        }
                    }
                }
            }
        };

        let runTimeout, launchTimeout;
        
        function cleanup() {
            clearTimeout(runTimeout);
            clearTimeout(launchTimeout);
            clearInterval(instrumentPoller);
        };

        let finished = false;
        function finish(success, reason, code = success ? 'OK' : 'FAILED', extra) {
            if (finished) return;
            finished = true;
            cleanup();

            for (const snap of xsdbConfigSnapshots) {
                try {
                    if (snap.existed && snap.original !== null) {
                        fs.writeFileSync(snap.path, snap.original);
                    } else if (!snap.existed && fs.existsSync(snap.path)) {
                        fs.unlinkSync(snap.path);
                    }
                } catch {}
            }
            
            if (child.stdin && child.stdin.writable) {
                try { child.stdin.write("quit\n"); } catch {}
            }
            
            // Allow 500ms grace period for hardware port to cleanly unbind via OS drivers
            setTimeout(() => {
                try { child.kill('SIGKILL'); } catch {}
                try { 
                     require('child_process').execSync('killall -9 mcsim 2>/dev/null', { stdio: 'ignore', timeout: 2000 }); 
                     require('child_process').execSync('killall -9 xsl 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
                     require('child_process').execSync('killall -9 serial2xsbug 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
                     require('child_process').execSync('pkill -9 -f serial2xsbug 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
                     require('child_process').execSync('pkill -9 -f xsbug-log 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
                     require('child_process').execSync('pkill -9 -f xsdb 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
                } catch {}
                
                let lines = outputBuf.replace(/\r/g, '\n').split('\n');
                let cleanLog = [];
                for (let i = 0; i < lines.length; i++) {
                    let line = lines[i];
                    let isSpam = line.startsWith('Writing at 0x') || /^\.* \(\d+ %\)$/.test(line) || line === '.';
                    if (isSpam) {
                        let nextLine = (i + 1 < lines.length) ? lines[i+1] : '';
                        let nextSpam = nextLine.startsWith('Writing at 0x') || /^\.* \(\d+ %\)$/.test(nextLine) || nextLine === '.';
                        if (!nextSpam) cleanLog.push(line);
                    } else {
                        cleanLog.push(line);
                    }
                }
                let finalLog = cleanLog.join('\n').replace(/\n{3,}/g, '\n\n');
                
                resolve({ success, code, reason, ...extra, log: finalLog, durationMs: Date.now() - startTime });
            }, 500);
        };
        
        // Build phase wrapper: 3 minutes max
        runTimeout = setTimeout(() => {
            finish(false, 'Build timeout (exceeded 3 minutes)', 'BUILD_TIMEOUT');
        }, 3 * 60 * 1000);

        child.stdout.on('data', (data) => {
            const str = data.toString();
            outputBuf += str;
            // esptool's message can straddle two chunks, so match on a window of recent output
            recentOutput = (recentOutput + str).slice(-8192);

            if (/failed to connect to esp\w*|no serial data received/i.test(recentOutput)) {
                exceptionOccurred = true;
                finish(false, describeBuildFailure(recentOutput) || 'Flash failed: the device did not respond', 'SERIAL_FAIL');
                return;
            }

            if (str.includes('y / n)')) {
                child.stdin.write("y\n");
            }



            // Setup xsdb on first connection
            if (!isSetup && (outputBuf.includes('(xsdb)') || outputBuf.includes('xsdb listening on port'))) {
                isSetup = true;
                
                // Switch from Build Timeout to Launch Timeout
                clearTimeout(runTimeout);
                // Give it 30 seconds to launch and collect data (plus extra 30s if it needs wifi connection)
                const timeoutMs = isNet && isEmbedded ? 60000 : 30000;
                launchTimeout = setTimeout(() => {
                    const diagnostics = {
                        connected: connectCount > 0,
                        restarts: Math.max(0, connectCount - 1),
                        disconnected: disconnectCount > 0,
                        samples: sampleCount,
                        stalledForSeconds: lastSampleTime ? Math.round((Date.now() - lastSampleTime) / 1000) : 0,
                        waitingForIP: isNet && isEmbedded && !ipDetected,
                        lastMessage: lastTargetMessage
                    };
                    finish(false, describeRunTimeout(Math.round(timeoutMs / 1000), diagnostics), 'RUN_TIMEOUT', { diagnostics });
                }, timeoutMs);

                // Start polling instruments
                instrumentPoller = setInterval(() => {
                    child.stdin.write("info instruments\n");
                }, 1000);
            }

            // Once IP is found on a network test. Only meaningful once the target is running:
            // build output (paths, tool messages) can otherwise match by accident.
            if (!ipDetected && !onlyBuild && isSetup && str.match(/\b(?:IP address:?\s*|IP:?\s*)([0-9a-fA-F:.]+)/i)) {
                console.log("   [Test Harness] IP Address detected.");
                ipDetected = true;
            }

            // Stream parse JSON blocks
            for (let i = 0; i < str.length; i++) {
                const c = str[i];

                if (inJson) {
                    if (escapeNext) {
                        escapeNext = false;
                    } else if (c === '\\') {
                        escapeNext = true;
                    } else if (c === '"') {
                        inString = !inString;
                    }
                }

                if (!inString) {
                    if (c === '{') {
                        if (openBraces === 0) {
                            inJson = true;
                            jsonBuffer = '';
                        }
                        openBraces++;
                    }
                }

                if (inJson) {
                    jsonBuffer += c;
                }

                if (!inString && c === '}') {
                    openBraces--;
                    if (openBraces === 0 && inJson) {
                        inJson = false;
                        try {
                            const obj = JSON.parse(jsonBuffer);
                            handleEvent(obj);
                        } catch {}
                    }
                }
            }
        });

        child.stderr.on('data', (data) => {
            const str = data.toString();
            outputBuf += str;
            recentOutput = (recentOutput + str).slice(-8192);
            // Removed process.stdout.write
            
            if (/exception[:,]/i.test(str) && !str.includes('Break on exceptions:')) {
                exceptionOccurred = true;
                const line = str.replace(ANSI, '').split(/[\r\n]+/).find(l => /exception[:,]/i.test(l));
                finish(false, line ? 'Exception: ' + trimBang(line) : 'Exception reported on stderr', 'EXCEPTION');
            }

            if (/failed to connect to esp\w*|no serial data received/i.test(recentOutput)) {
                exceptionOccurred = true;
                finish(false, describeBuildFailure(recentOutput) || 'Flash failed: the device did not respond', 'SERIAL_FAIL');
            }
        });

        child.on('close', (code) => {
            if (!exceptionOccurred) {
                if (onlyBuild && code === 0) {
                    finish(true, mode === 'build' ? 'Build successful (Launch bypassed via --mode build)' : 'Build successful (Launch skipped due to missing Wi-Fi credentials)');
                } else {
                    // Say what the build printed, rather than only the exit code it returned.
                    const why = describeBuildFailure(outputBuf);
                    const last = why ? '' : lastMeaningfulLine(outputBuf);
                    // A clean exit here means the tool did its job and quit without ever
                    // running the application — a usage error, most likely.
                    const summary = (code === 0) ? `${buildCmd} exited without launching the application` : `${buildCmd} exited with code ${code}`;
                    finish(false, why || (last ? `${summary}: ${clip(last)}` : summary), 'BUILD_FAILED', { exitCode: code });
                }
            }
        });
        };

        const runCleanThenBuild = () => {
            if (!cleanBuild) {
                runMainBuild();
                return;
            }
            console.log(`   [Test Harness] Cleaning target ${examplePath}...`);
            const cleanArgs = ['-d', '-t', 'clean', '-m', '-p', target];
            const cleanChild = spawn(buildCmd, [...buildPrefix, ...cleanArgs], {
                cwd: examplePath,
                env: process.env,
                shell: process.platform === 'win32'
            });
            cleanChild.on('close', (code) => {
                if (code !== 0) {
                    resolve({ success: false, code: 'CLEAN_FAILED', reason: `Clean failed (${buildCmd} -t clean exited with code ${code})`, log: `${buildCmd} clean exited with code ${code}`, durationMs: Date.now() - startTime });
                } else {
                    runMainBuild();
                }
            });
        };

        const runPackageSteps = (next) => {
            if (!isPackage) {
                next();
                return;
            }
            const hasDeps = packageJson && packageJson.dependencies && Object.keys(packageJson.dependencies).length > 0;
            const hasBuildScript = !!(packageJson && packageJson.scripts && packageJson.scripts.build);
            const nodeModulesPath = path.join(examplePath, 'node_modules');
            const needInstall = hasDeps && !fs.existsSync(nodeModulesPath);

            const spawnStep = (label, cmd, args, after) => {
                console.log(`   [Test Harness] Running ${label} in ${examplePath}...`);
                let logBuf = '';
                const stepChild = spawn(cmd, args, {
                    cwd: examplePath,
                    env: process.env,
                    shell: process.platform === 'win32'
                });
                stepChild.stdout.on('data', (d) => { logBuf += d.toString(); });
                stepChild.stderr.on('data', (d) => { logBuf += d.toString(); });
                stepChild.on('error', (err) => {
                    resolve({ success: false, code: 'STEP_FAILED', reason: `${label} failed: ${clip(err.message)}`, log: `${err.message}\n${logBuf}`, durationMs: Date.now() - startTime });
                });
                stepChild.on('close', (code) => {
                    if (code !== 0) {
                        const why = describeBuildFailure(logBuf) || lastMeaningfulLine(logBuf);
                        resolve({ success: false, code: 'STEP_FAILED', reason: why ? `${label} failed: ${clip(why)}` : `${label} failed (exit code ${code})`, log: `${cmd} ${args.join(' ')} exited with code ${code}\n${logBuf}`, durationMs: Date.now() - startTime });
                        return;
                    }
                    after();
                });
            };

            const runBuildScript = () => {
                if (!hasBuildScript) {
                    next();
                    return;
                }
                spawnStep('npm run build', 'npm', ['run', 'build'], next);
            };

            if (needInstall) {
                spawnStep('npm install', 'npm', ['install'], runBuildScript);
            } else {
                runBuildScript();
            }
        };

        runPackageSteps(runCleanThenBuild);
    });
}

const runStarted = new Date();
const reportPath = path.join(moddableDir, 'tools', 'test-examples', 'report.json');

// Identifies the tree under test, so a report can be matched to what produced it.
function moddableVersion() {
    try {
        const git = (cmd) => require('child_process').execSync(cmd, { cwd: moddableDir, encoding: 'utf-8', stdio: ['ignore', 'pipe', 'ignore'] }).trim();
        const commit = git('git rev-parse --short HEAD');
        return git('git status --porcelain') ? `${commit}-dirty` : commit;
    } catch {
        return undefined;
    }
}

function isSkipped(result) {
    // 'reason' held the code in reports written before the header was added.
    return (result.code === 'NO_BASE_MANIFEST') || (result.reason === 'NO_BASE_MANIFEST');
}

// The report leads with the settings of the run that produced it: which device,
// which tree, and when. Everything after that is per-example.
function writeReport(results, state) {
    const finished = new Date();
    const report = {
        run: {
            tool: 'test-examples',
            started: runStarted.toISOString(),
            startedLocal: runStarted.toString(),
            finished: finished.toISOString(),
            durationMs: finished - runStarted,
            interrupted: !!(state && state.interrupted),
            resumed: isContinue
        },
        settings: {
            target,
            targets: target.includes('*') ? targets : undefined,
            device: deviceInfo || (process.env.UPLOAD_PORT ? { port: process.env.UPLOAD_PORT } : undefined),
            mode: mode || 'run',
            cleanBuild,
            dir: specificExample ? undefined : examplesDir,
            example: specificExample || undefined,
            ssid: ssid || undefined,
            password: password ? '(provided)' : undefined,
            moddable: moddableDir,
            moddableVersion: moddableVersion(),
            idfPath: target.startsWith('esp32/') ? process.env.IDF_PATH : undefined,
            host: `${process.platform} ${process.arch}, node ${process.version}`
        },
        summary: {
            total: results.length,
            passed: results.filter(r => r.success === true).length,
            failed: results.filter(r => r.success === false).length,
            skipped: results.filter(isSkipped).length,
            remaining: (state && state.remaining) || 0
        },
        results
    };

    fs.writeFileSync(reportPath, JSON.stringify(report, null, 2));
}

async function main() {
    const examples = specificExample ? [specificExample] : findExamples(examplesDir);
    let jobs = examples.flatMap(ex => targets.map(t => ({ example: ex, target: t })));
    let passes = 0;
    let fails = 0;
    let results = [];

    if (isContinue && fs.existsSync(reportPath)) {
        try {
            const past = JSON.parse(fs.readFileSync(reportPath, 'utf8'));
            // Reports written before the header was added are a bare array of results.
            const pastData = Array.isArray(past) ? past : past.results;
            if (Array.isArray(pastData)) {
                results = pastData;
                // Results written before targets could vary within a run carry no 'target'.
                const completed = new Set(results.map(r => `${r.path ? path.join(moddableDir, r.path) : ''}|${r.target || target}`));
                jobs = jobs.filter(job => !completed.has(`${job.example}|${job.target}`));
                passes = results.filter(r => r.success === true).length;
                fails = results.filter(r => r.success === false).length;
                let skips = results.filter(isSkipped).length;
                console.log(`[Test Harness] --continue flag set. Resuming from report.json.`);
                console.log(`[Test Harness] Skipping ${results.length} previously tested examples (${passes} passed, ${fails} failed, ${skips} explicitly bypassed).\n`);
            }
        } catch {
            console.warn(`[Test Harness] Failed to parse previous report.json. Starting fresh.`);
        }
    }

    let initialPasses = passes;
    let initialFails = fails;
    let initialSkips = results.filter(isSkipped).length;

    process.on('SIGINT', () => {
        let currentSkips = results.filter(isSkipped).length;
        let completedThisRun = (passes - initialPasses) + (fails - initialFails) + (currentSkips - initialSkips);
        let aborted = jobs.length - completedThisRun;
        console.log('\n\n=====================================');
        console.log(`TEST RUN INTERRUPTED BY USER (^C)`);
        console.log(`Total Passed: ${passes}, Total Failed: ${fails}, Bypassed: ${currentSkips}, Aborted Remaining: ${aborted}`);
        console.log('=====================================');
        writeReport(results, { interrupted: true, remaining: aborted });
        
        try { 
            require('child_process').execSync('killall -9 mcsim 2>/dev/null', { stdio: 'ignore', timeout: 2000 }); 
            require('child_process').execSync('pkill -9 -f xsbug-log 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
            require('child_process').execSync('pkill -9 -f xsdb 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
        } catch {}

        process.exit(1);
    });

    for (let job of jobs) {
        const ex = job.example;
        const res = await runTest(ex, job.target);
        const durationSecs = (res.durationMs / 1000).toFixed(1);
        if (isSkipped(res)) {
            console.log(`⏭️  SKIPPED - Target missing 'manifest_base.json' (likely a library).`);
        } else if (res.success === true) {
            console.log(`✅ PASS (${durationSecs}s)`);
            passes++;
        } else {
            console.log(`❌ FAIL - ${res.reason} (${durationSecs}s)`);
            if (res.log) {
                console.log(`\n--- BUILD LOG ---\n${res.log.substring(res.log.length - 2000)}\n-----------------\n`);
            }
            fails++;
        }
        if (res.success !== false)
            delete res.log;        // keep report.json small: full logs are only of interest for failures
        results.push({ path: path.relative(moddableDir, ex), dir: path.dirname(ex), name: path.basename(ex), target: job.target, ...res });
        
        if (res.code === 'SERIAL_FAIL') {
            console.log(`\nFATAL ERROR: Hardware serial communication failure detected.`);
            console.log(`This is strongly indicative of the underlying OS serial drivers locking up or being actively held by a lingering process.`);
            console.log(`Exiting...\n`);
            break;
        }
    }

    let endSkips = results.filter(isSkipped).length;
    console.log('\n=====================================');
    console.log(`TEST RUN COMPLETE. Passed: ${passes}, Failed: ${fails}, Skipped/Bypassed: ${endSkips}`);
    console.log('=====================================');
    
    writeReport(results);

    process.exit(fails > 0 ? 1 : 0);
}

main();
