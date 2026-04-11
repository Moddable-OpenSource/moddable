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

Usage: node index.js <target> [options]

Arguments:
  <target>                Platform target to build for (e.g., mac, sim/moddable_six, esp32/moddable_six).

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
  'report.json' is output to tools/test-examples/ on completion.
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

const isEmbedded = !['mac', 'win', 'lin'].includes(target.toLowerCase()) && target.toLowerCase() !== 'sim' && !target.toLowerCase().startsWith('sim/');
const examplesDir = customDir ? path.resolve(customDir) : path.join(moddableDir, 'examples');

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
            process.stdout.write("Probing ESP32 hardware via esptool.py... ");
            const portArg = process.env.UPLOAD_PORT ? ` --port ${process.env.UPLOAD_PORT}` : '';
            const esptoolOut = require('child_process').execSync(`esptool.py${portArg} chip_id`, { encoding: 'utf-8', env: process.env, stdio: ['ignore', 'pipe', 'ignore'] });
            const portMatch = esptoolOut.match(/Serial port\s+([^\r\n:]+)/);
            const chipMatch = esptoolOut.match(/Chip is\s+(.+)/);
            const featuresMatch = esptoolOut.match(/Features:\s+(.+)/);
            
            if (portMatch) {
                process.env.UPLOAD_PORT = portMatch[1].trim();
            }
            
            console.log("Done.");
            if (chipMatch) console.log(`   Chip: ${chipMatch[1].trim()}`);
            if (featuresMatch) console.log(`   Features: ${featuresMatch[1].trim()}`);
            if (portMatch) console.log(`   Port: ${process.env.UPLOAD_PORT}`);
        } catch {
            const portHelp = process.env.UPLOAD_PORT ? ` on $UPLOAD_PORT (${process.env.UPLOAD_PORT})` : '';
            console.error(`\nError: Failed to probe ESP32 via esptool.py${portHelp}. Is the device connected?\nUse '--mode build' to test builds without a device.`);
            process.exit(1);
        }
    }
}

function findExamples(dir) {
    let results = [];
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
        if (entry.name.startsWith('.')) continue;
        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            const manifestPath = path.join(fullPath, 'manifest.json');
            if (fs.existsSync(manifestPath)) {
                results.push(fullPath);
            } else {
                results = results.concat(findExamples(fullPath));
            }
        }
    }
    return results;
}

function needsNetwork(manifestPath) {
    try {
        const raw = fs.readFileSync(manifestPath, 'utf-8');
        return raw.includes('manifest_net.json');
    } catch {
        return false;
    }
}

function runTest(examplePath) {
    return new Promise((resolve) => {
        const startTime = Date.now();
        const manifestPath = path.join(examplePath, 'manifest.json');
        
        let manifestRaw = '';
        try {
            manifestRaw = fs.readFileSync(manifestPath, 'utf-8');
        } catch {}
        
        if (!manifestRaw.includes('manifest_base.json')) {
            return resolve({ reason: 'NO_BASE_MANIFEST', log: '', durationMs: 0 });
        }
        
        const isNet = manifestRaw.includes('manifest_net.json');
        
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
        
        console.log(`\n========= Testing ${examplePath} =========`);

        let originalXsdbConfig = null;
        let xsdbConfigExisted = false;
        const xsdbConfigPath = require('path').join(examplePath, '.xsdb.json');
        
        try {
            if (require('fs').existsSync(xsdbConfigPath)) {
                xsdbConfigExisted = true;
                originalXsdbConfig = require('fs').readFileSync(xsdbConfigPath, 'utf8');
            }
            require('fs').writeFileSync(xsdbConfigPath, JSON.stringify({ exceptionsMode: 'off', outputFormat: 'json' }));
        } catch {}

        const runMainBuild = () => {
            const child = spawn('mcconfig', mcArgs, {
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
        
        let jsonBuffer = '';
        let openBraces = 0;
        let inJson = false;
        let inString = false;
        let escapeNext = false;
        
        let pendingAbortText = null;
        
        function handleEvent(obj) {
            if (obj.event === 'stopped') {
                if (pendingAbortText) {
                    exceptionOccurred = true;
                    finish(false, 'XS Abort detected: ' + pendingAbortText);
                    return;
                }
                if (obj.data && typeof obj.data.reason === 'string') {
                    const reason = obj.data.reason;
                    if (reason === '# Break: breakpoint!' || reason === '# Break: step!' || reason === '# Break: debugger!') {
                        // Resumes from deliberate programmatic breakpoints
                        child.stdin.write("c\n");
                    } else if (reason.startsWith('# Break:')) {
                        // Any other break is an unhandled exception or error (SyntaxError, TypeError, etc)
                        exceptionOccurred = true;
                        finish(false, 'Exception detected: ' + reason.replace('# Break: ', '').trim());
                    } else {
                        child.stdin.write("c\n");
                    }
                } else {
                    child.stdin.write("c\n");
                }
            } else if (obj.event === 'log') {
                if (obj.data && typeof obj.data.text === 'string' && obj.data.text.includes('XS abort')) {
                    exceptionOccurred = true;
                    finish(false, 'XS Abort detected');
                }
            } else if (obj.event === 'info_instruments') {
                if (ipDetected && obj.data && typeof obj.data.historyCount === 'number') {
                    if (obj.data.historyCount > previousHistoryCount) {
                        previousHistoryCount = obj.data.historyCount;
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
        function finish(success, reason) {
            if (finished) return;
            finished = true;
            cleanup();

            try {
                if (xsdbConfigExisted && originalXsdbConfig !== null) {
                    require('fs').writeFileSync(xsdbConfigPath, originalXsdbConfig);
                } else if (!xsdbConfigExisted && require('fs').existsSync(xsdbConfigPath)) {
                    require('fs').unlinkSync(xsdbConfigPath);
                }
            } catch {}
            
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
                
                resolve({ success, reason, log: finalLog, durationMs: Date.now() - startTime });
            }, 500);
        };
        
        // Build phase wrapper: 3 minutes max
        runTimeout = setTimeout(() => {
            finish(false, 'Build timeout (exceeded 3 minutes)');
        }, 3 * 60 * 1000);

        child.stdout.on('data', (data) => {
            const str = data.toString();
            outputBuf += str;

            if (/failed to connect to esp32|no serial data received/i.test(str)) {
                exceptionOccurred = true;
                finish(false, 'HARD_SERIAL_FAIL');
                return;
            }

            if (str.includes('y / n)')) {
                child.stdin.write("y\n");
            }

            // Once IP is found on a network test
            if (!ipDetected && str.match(/(?:IP address:?\s*|IP:?\s*)([0-9a-fA-F:.]+)/i)) {
                console.log("   [Test Harness] IP Address detected.");
                ipDetected = true;
            }



            // Setup xsdb on first connection
            if (!isSetup && (outputBuf.includes('(xsdb)') || outputBuf.includes('xsdb listening on port'))) {
                isSetup = true;
                
                // Switch from Build Timeout to Launch Timeout
                clearTimeout(runTimeout);
                // Give it 30 seconds to launch and collect data (plus extra 30s if it needs wifi connection)
                const timeoutMs = isNet && isEmbedded ? 60000 : 30000;
                launchTimeout = setTimeout(() => {
                    let reason = 'Run timeout (application hung or no instrumentation received)';
                    if (isNet && isEmbedded && !ipDetected) {
                        reason = 'Run timeout (Failed to get IP address)';
                    }
                    finish(false, reason);
                }, timeoutMs);

                // Start polling instruments
                instrumentPoller = setInterval(() => {
                    child.stdin.write("info instruments\n");
                }, 1000);
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
            // Removed process.stdout.write
            
            if (/exception[:,]/i.test(str) && !str.includes('Break on exceptions:')) {
                exceptionOccurred = true;
                finish(false, 'Exception detected');
            }

            if (/failed to connect to esp32|no serial data received/i.test(str)) {
                exceptionOccurred = true;
                finish(false, 'HARD_SERIAL_FAIL');
            }
        });

        child.on('close', (code) => {
            if (!exceptionOccurred) {
                if (onlyBuild && code === 0) {
                    finish(true, mode === 'build' ? 'Build successful (Launch bypassed via --mode build)' : 'Build successful (Launch skipped due to missing Wi-Fi credentials)');
                } else {
                    finish(false, `Closed unexpectedly with code ${code}`);
                }
            }
        });
        };

        if (cleanBuild) {
            console.log(`   [Test Harness] Cleaning target ${examplePath}...`);
            const cleanArgs = ['-d', '-t', 'clean', '-m', '-p', target];
            const cleanChild = spawn('mcconfig', cleanArgs, {
                cwd: examplePath,
                env: process.env,
                shell: process.platform === 'win32'
            });
            cleanChild.on('close', (code) => {
                if (code !== 0) {
                    resolve({ success: false, reason: 'Clean failed', log: `mcconfig clean exited with code ${code}`, durationMs: Date.now() - startTime });
                } else {
                    runMainBuild();
                }
            });
        } else {
            runMainBuild();
        }
    });
}

async function main() {
    let examples = specificExample ? [specificExample] : findExamples(examplesDir);
    let passes = 0;
    let fails = 0;
    let results = [];

    const reportPath = path.join(moddableDir, 'tools', 'test-examples', 'report.json');

    if (isContinue && fs.existsSync(reportPath)) {
        try {
            const pastData = JSON.parse(fs.readFileSync(reportPath, 'utf8'));
            if (Array.isArray(pastData)) {
                results = pastData;
                const completedPaths = new Set(results.map(r => r.path ? path.join(moddableDir, r.path) : ''));
                examples = examples.filter(ex => !completedPaths.has(ex));
                passes = results.filter(r => r.success === true).length;
                fails = results.filter(r => r.success === false).length;
                let skips = results.filter(r => r.reason === 'NO_BASE_MANIFEST').length;
                console.log(`[Test Harness] --continue flag set. Resuming from report.json.`);
                console.log(`[Test Harness] Skipping ${results.length} previously tested examples (${passes} passed, ${fails} failed, ${skips} explicitly bypassed).\n`);
            }
        } catch {
            console.warn(`[Test Harness] Failed to parse previous report.json. Starting fresh.`);
        }
    }

    let initialPasses = passes;
    let initialFails = fails;
    let initialSkips = results.filter(r => r.reason === 'NO_BASE_MANIFEST').length;

    process.on('SIGINT', () => {
        let currentSkips = results.filter(r => r.reason === 'NO_BASE_MANIFEST').length;
        let completedThisRun = (passes - initialPasses) + (fails - initialFails) + (currentSkips - initialSkips);
        let aborted = examples.length - completedThisRun;
        console.log('\n\n=====================================');
        console.log(`TEST RUN INTERRUPTED BY USER (^C)`);
        console.log(`Total Passed: ${passes}, Total Failed: ${fails}, Bypassed: ${currentSkips}, Aborted Remaining: ${aborted}`);
        console.log('=====================================');
        fs.writeFileSync(path.join(moddableDir, 'tools', 'test-examples', 'report.json'), JSON.stringify(results, null, 2));
        
        try { 
            require('child_process').execSync('killall -9 mcsim 2>/dev/null', { stdio: 'ignore', timeout: 2000 }); 
            require('child_process').execSync('pkill -9 -f xsbug-log 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
            require('child_process').execSync('pkill -9 -f xsdb 2>/dev/null', { stdio: 'ignore', timeout: 2000 });
        } catch {}

        process.exit(1);
    });

    for (let ex of examples) {
        const res = await runTest(ex);
        const durationSecs = (res.durationMs / 1000).toFixed(1);
        if (res.reason === 'NO_BASE_MANIFEST') {
            console.log(`⏭️  SKIPPED - Target missing 'manifest_base.json' (likely a library).`);
        } else if (res.success === true) {
            console.log(`✅ PASS (${durationSecs}s)`);
            passes++;
        } else {
            console.log(`❌ FAIL - ${res.reason === 'HARD_SERIAL_FAIL' ? 'Hardware Serial Lock Detected' : res.reason} (${durationSecs}s)`);
            if (res.log) {
                console.log(`\n--- BUILD LOG ---\n${res.log.substring(res.log.length - 2000)}\n-----------------\n`);
            }
            fails++;
        }
        results.push({ path: path.relative(moddableDir, ex), dir: path.dirname(ex), name: path.basename(ex), ...res });
        
        if (res.reason === 'HARD_SERIAL_FAIL') {
            console.log(`\nFATAL ERROR: Hardware serial communication failure detected.`);
            console.log(`This is strongly indicative of the underlying OS serial drivers locking up or being actively held by a lingering process.`);
            console.log(`Exiting...\n`);
            break;
        }
    }

    let endSkips = results.filter(r => r.reason === 'NO_BASE_MANIFEST').length;
    console.log('\n=====================================');
    console.log(`TEST RUN COMPLETE. Passed: ${passes}, Failed: ${fails}, Skipped/Bypassed: ${endSkips}`);
    console.log('=====================================');
    
    // Optional: write a report file
    fs.writeFileSync(path.join(moddableDir, 'tools', 'test-examples', 'report.json'), JSON.stringify(results, null, 2));

    process.exit(fails > 0 ? 1 : 0);
}

main();
