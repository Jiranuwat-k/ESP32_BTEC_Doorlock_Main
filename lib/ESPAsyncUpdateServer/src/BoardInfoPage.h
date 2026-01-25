#ifndef BOARDINFOPAGE_H
#define BOARDINFOPAGE_H
static const char boardInfoPage[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Board Info</title>
    <style>
        * { box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; display: flex; flex-direction: column; align-items: center; min-height: 100vh; background-color: #f4f4f9; margin: 0; padding: 20px; color: #333; position: relative; }
        .container { width: 100%; max-width: 450px; background: white; padding: 25px; border-radius: 16px; box-shadow: 0 10px 25px rgba(0,0,0,0.1); display: flex; flex-direction: column; margin: auto; position: relative; z-index: 10; text-align: center; }
        h2 { margin-bottom: 0; margin-top: 0.5rem; color: #2c3e50; }
        h3 { font-size: 1.1rem; color: #555; margin: 1.5rem 0 0.5rem 0; text-align: left; border-bottom: 1px solid #eee; padding-bottom: 0.5rem; }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
            gap: 10px;
            text-align: left;
            font-size: 0.9rem;
        }
        .info-item { background: #f8f9fa; padding: 10px; border-radius: 6px; }
        .info-label { font-weight: 600; color: #666; font-size: 0.8rem; display: block; margin-bottom: 2px; }
        .info-value { font-family: monospace; color: #333; font-size: 1rem; word-break: break-all; }

        @media (max-width: 480px) {
            body { padding: 10px; }
            .container { padding: 15px; }
            .info-grid { grid-template-columns: 1fr; }
        }
        
        .partition-bar {
            display: flex;
            width: 100%;
            height: 30px;
            background-color: #eee;
            border-radius: 4px;
            overflow: hidden;
            margin: 1rem 0;
        }
        .part-segment { height: 100%; display: flex; align-items: center; justify-content: center; color: #fff; font-size: 0.75rem; font-weight: bold; }
        .part-sketch { background-color: #007bff; }
        .part-free { background-color: #28a745; }
        .part-fs { background-color: #ff9800; }
        .part-other { background-color: #6c757d; }

        .legend {
            display: flex;
            justify-content: center;
            gap: 15px;
            font-size: 0.8rem;
            margin-bottom: 1.5rem;
        }
        .legend-item { display: flex; align-items: center; gap: 5px; }
        .dot { width: 10px; height: 10px; border-radius: 50%; }
        
        .btn { display: inline-block; background-color: #6c757d; color: white; text-decoration: none; padding: 0.6rem 1.2rem; border-radius: 6px; margin-top: 1.5rem; transition: background 0.2s; font-size: 0.9rem; }
        .btn:hover { background-color: #5a6268; }

        /* Copyright Footer */
        .copyright {
            position: relative;
            margin-top: 20px;
            font-size: 12px;
            color: #999;
            text-align: center;
            width: 100%;
            padding-left: 60px;
            padding-right: 60px;
            box-sizing: border-box;
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>Device Information</h2>
        
        <h3>System</h3>
        <div class="info-grid">
            <div class="info-item"><span class="info-label">Board</span><span class="info-value" id="board_name">-</span></div>
            <div class="info-item"><span class="info-label">Chip ID</span><span class="info-value" id="chip_id">-</span></div>
            <div class="info-item"><span class="info-label">MAC Address</span><span class="info-value" id="mac_addr">-</span></div>
            <div class="info-item"><span class="info-label">CPU Freq</span><span class="info-value" id="cpu_freq">-</span></div>
            <div class="info-item"><span class="info-label">Core Version</span><span class="info-value" id="core_ver">-</span></div>
            <div class="info-item"><span class="info-label">SDK Version</span><span class="info-value" id="sdk_ver">-</span></div>
            <div class="info-item"><span class="info-label">Free Heap</span><span class="info-value" id="free_heap">-</span></div>
            <div class="info-item"><span class="info-label">Heap Frag</span><span class="info-value" id="heap_frag">-</span></div>
            <div class="info-item"><span class="info-label">Reset Reason</span><span class="info-value" id="reset_reason">-</span></div>
        </div>

        <h3>Software</h3>
        <div class="info-grid">
             <div class="info-item"><span class="info-label">App Name</span><span class="info-value" id="app_name">-</span></div>
             <div class="info-item"><span class="info-label">Version</span><span class="info-value" id="app_version">-</span></div>
             <div class="info-item" style="grid-column: span 2;"><span class="info-label">Firmware File</span><span class="info-value" id="firmware_file" style="font-size: 0.8rem; word-break: break-all;">-</span></div>
        </div>

        <h3>Network</h3>
        <div class="info-grid">
             <div class="info-item"><span class="info-label">Mode</span><span class="info-value" id="wifi_mode">-</span></div>
             <div class="info-item"><span class="info-label">Signal</span><span class="info-value" id="rssi">-</span></div>
             <div class="info-item"><span class="info-label">SSID</span><span class="info-value" id="ssid">-</span></div>
             <div class="info-item"><span class="info-label">Hostname</span><span class="info-value" id="hostname">-</span></div>
             <div class="info-item"><span class="info-label">IP Address</span><span class="info-value" id="ip_addr">-</span></div>
             <div class="info-item"><span class="info-label">Subnet</span><span class="info-value" id="subnet">-</span></div>
             <div class="info-item"><span class="info-label">Gateway</span><span class="info-value" id="gateway">-</span></div>
             <div class="info-item"><span class="info-label">DNS</span><span class="info-value" id="dns" style="font-size: 0.8rem;">-</span></div>
        </div>

        <h3>Storage</h3>
        <div class="partition-bar" id="partition_bar"></div>
        <div class="legend">
            <div class="legend-item"><div class="dot" style="background:#007bff"></div>Sketch</div>
            <div class="legend-item"><div class="dot" style="background:#28a745"></div>Free Space</div>
            <div class="legend-item"><div class="dot" style="background:#ff9800"></div>FileSystem</div>
            <div class="legend-item"><div class="dot" style="background:#6c757d"></div>Other</div>
        </div>
        <div class="info-grid">
            <div class="info-item"><span class="info-label">Flash Size</span><span class="info-value" id="chip_size">-</span></div>
            <div class="info-item"><span class="info-label">Sketch Size</span><span class="info-value" id="sketch_size">-</span></div>
            <div class="info-item"><span class="info-label">Free Space</span><span class="info-value" id="free_sketch_space">-</span></div>
            <div class="info-item"><span class="info-label">FS Used</span><span class="info-value" id="fs_stats">-</span></div>
            <div class="info-item"><span class="info-label">Flash Speed</span><span class="info-value" id="flash_speed">-</span></div>
            <div class="info-item"><span class="info-label">Flash Real Size</span><span class="info-value" id="flash_real_size">-</span></div>
        </div>
        
        <h3>Build</h3>
        <div class="info-grid" style="grid-template-columns: 1fr;">
            <div class="info-item"><span class="info-label">Approx. Compile Time</span><span class="info-value"><span id="compile_date"></span> <span id="compile_time"></span></span></div>
        </div>

        <a href="/update" class="btn">&larr; Back to Update</a>
    </div>
    
    <div class="copyright">
        &copy; 2025 WheelClimb@KMUTNB | Developed by Jiranuwat.k
    </div>
    
    <script>
        function formatBytes(bytes, decimals = 2) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const dm = decimals < 0 ? 0 : decimals;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
        }

        fetch('/info')
            .then(response => response.json())
            .then(data => {
                // System
                document.getElementById('board_name').innerText = data.board || '-';
                document.getElementById('chip_id').innerText = data.chip_id || '-';
                document.getElementById('mac_addr').innerText = data.mac || '-';
                document.getElementById('cpu_freq').innerText = (data.cpu_freq || '-') + " MHz";
                document.getElementById('core_ver').innerText = data.core_ver || '-';
                document.getElementById('sdk_ver').innerText = data.sdk_ver || '-';
                document.getElementById('free_heap').innerText = formatBytes(data.free_heap);
                document.getElementById('heap_frag').innerText = (data.heap_frag || 0) + "%";
                document.getElementById('reset_reason').innerText = data.reset_reason || '-';
                
                // Software & Build
                document.getElementById('app_name').innerText = data.app_name || '-';
                document.getElementById('app_version').innerText = data.app_version || '-';
                document.getElementById('firmware_file').innerText = data.firmware_file || '-';
                document.getElementById('compile_date').innerText = data.compile_date || '';
                document.getElementById('compile_time').innerText = data.compile_time || '';

                // Network
                document.getElementById('wifi_mode').innerText = data.wifi_mode || '-';
                document.getElementById('rssi').innerText = (data.rssi || '-') + " dBm";
                document.getElementById('ssid').innerText = data.ssid || '-';
                document.getElementById('hostname').innerText = data.hostname || '-';
                document.getElementById('ip_addr').innerText = data.ip || '-';
                document.getElementById('subnet').innerText = data.subnet || '-';
                document.getElementById('gateway').innerText = data.gateway || '-';
                document.getElementById('dns').innerText = (data.dns1 || '-') + " / " + (data.dns2 || '-');

                // Storage
                document.getElementById('chip_size').innerText = formatBytes(data.chip_size);
                document.getElementById('sketch_size').innerText = formatBytes(data.sketch_size);
                document.getElementById('free_sketch_space').innerText = formatBytes(data.free_sketch_space);
                document.getElementById('fs_stats').innerText = formatBytes(data.fs_used) + " / " + formatBytes(data.fs_size);
                document.getElementById('flash_speed').innerText = formatBytes(data.flash_speed) || '-';
                document.getElementById('flash_real_size').innerText = formatBytes(data.flash_real_size) || '-';
                
                // Bar
                const total = data.chip_size;
                const sketchPct = (data.sketch_size / total) * 100;
                const freePct = (data.free_sketch_space / total) * 100;
                const fsPct = (data.fs_size / total) * 100;
                const otherPct = 100 - (sketchPct + freePct + fsPct);
                
                document.getElementById('partition_bar').innerHTML = `
                    <div class="part-segment part-sketch" style="width: ${sketchPct}%" title="Sketch">App</div>
                    <div class="part-segment part-free" style="width: ${freePct}%" title="Free">OTA</div>
                    <div class="part-segment part-fs" style="width: ${fsPct}%" title="FS">FS</div>
                    <div class="part-segment part-other" style="width: ${otherPct < 0 ? 0 : otherPct}%"></div>
                `;
            })
            .catch(err => console.error('Failed to load board info', err));
    </script>
</body>
</html>
)";
#endif
