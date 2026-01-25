#ifndef OTAWEBPAGE_H
#define OTAWEBPAGE_H

static const char serverIndex[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP Update</title>
    <style>
        * { box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; display: flex; flex-direction: column; align-items: center; min-height: 100vh; background-color: #f4f4f9; margin: 0; padding: 20px; color: #333; position: relative; }
        .container { width: 100%; max-width: 450px; background: white; padding: 25px; border-radius: 16px; box-shadow: 0 10px 25px rgba(0,0,0,0.1); display: flex; flex-direction: column; margin: auto; position: relative; z-index: 10; text-align: center; }
        h2 { margin-bottom: 0; margin-top: 0.5rem; color: #2c3e50; }
        
        .partition-bar {
            display: flex;
            width: 100%;
            height: 30px;
            background-color: #eee;
            border-radius: 4px;
            overflow: hidden;
            margin: 2rem 0 1rem 0;
            position: relative;
        }
        
        .part-segment {
            height: 100%;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #fff;
            font-size: 0.75rem;
            font-weight: bold;
            position: relative;
            transition: width 0.5s ease-in-out;
        }
        
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
        
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            text-align: left;
            font-size: 0.85rem;
            margin-bottom: 1.5rem;
            background: #f8f9fa;
            padding: 10px;
            border-radius: 8px;
        }
        
        .upload-section { margin-bottom: 1rem; text-align: left; }
        .upload-section label { display: block; margin-bottom: 0.3rem; font-weight: 600; font-size: 0.9rem; }
        .upload-section { margin-bottom: 1rem; text-align: left; }
        .upload-section label { display: block; margin-bottom: 0.3rem; font-weight: 600; font-size: 0.9rem; }
        .file-input-wrapper {
            display: flex;
            align-items: center;
            gap: 10px;
            border: 1px solid #ddd;
            background: #fafafa;
            padding: 5px;
            border-radius: 6px;
        }
        input[type="file"] { display: none; }
        .custom-file-btn {
            background-color: #6c757d; color: white; border: none; padding: 0.4rem 0.8rem; border-radius: 4px; cursor: pointer; font-size: 0.85rem; transition: background 0.2s; white-space: nowrap;
        }
        .custom-file-btn:hover { background-color: #5a6268; }
        .file-info { font-size: 0.85rem; color: #555; flex-grow: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
        
        input[type="submit"] { background-color: #007bff; color: white; border: none; padding: 0.8rem; border-radius: 6px; cursor: pointer; font-size: 1rem; width: 100%; transition: background 0.2s; font-weight: 600; margin-top: 1rem; }
        input[type="submit"]:hover { background-color: #0056b3; }
        input[type="submit"]:disabled { background-color: #ccc; cursor: not-allowed; }
        
        .progress-container { margin-top: 1rem; text-align: left; display: none; }
        .progress-label { font-size: 0.8rem; margin-bottom: 2px; display: flex; justify-content: space-between; }
        .progress-bar-bg { width: 100%; height: 10px; background: #e9ecef; border-radius: 5px; overflow: hidden; }
        .progress-bar-fill { height: 100%; background: #28a745; width: 0%; transition: width 0.2s; }
        
        /* Tooltip-like labels with lines */
        .part-label {
            position: absolute;
            top: -25px;
            font-size: 0.7rem;
            color: #333;
            white-space: nowrap;
        }
        .part-line {
            position: absolute;
            top: -5px;
            height: 5px;
            width: 1px;
            background: #333;
            left: 50%;
        }

        /* Footer Pills */
        .info-pill { padding: 4px 14px; border-radius: 2.5rem; font-weight: 500; font-size: 13px; }
        .chip-pill { background-color: #495057; color: #ffffff; font-family: monospace; }
        .board-pill { background-color: #3b82f6; color: white; font-family: monospace; }
        
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
        <h2>System OTA Update</h2>
        
        <div class="partition-bar" id="partition_bar">
            <!-- Segments injected by JS -->
        </div>
        
        <div class="legend">
            <div class="legend-item"><div class="dot" style="background:#007bff"></div>Sketch</div>
            <div class="legend-item"><div class="dot" style="background:#28a745"></div>Free Space</div>
            <div class="legend-item"><div class="dot" style="background:#ff9800"></div>FileSystem</div>
            <div class="legend-item"><div class="dot" style="background:#6c757d"></div>Other</div>
        </div>
        
        <div class="info-grid">
            <div><strong>Total Flash:</strong> <span id="chip_size">-</span></div>
            <div><strong>Sketch Size:</strong> <span id="sketch_size">-</span></div>
            <div><strong>Free Sketch:</strong> <span id="free_sketch_space">-</span></div>
            <div><strong>FS Size:</strong> <span id="fs_size">-</span></div>
            <div><strong>FS Used:</strong> <span id="fs_used">-</span></div>
        </div>

        <!-- Hidden form for logic handling only -->
        <form id='upload_form'>
            <div class="upload-section">
                <label>Firmware (.bin)</label>
                <div class="file-input-wrapper">
                    <label for="firmware_file" class="custom-file-btn">Choose File</label>
                    <input type='file' id='firmware_file' accept=".bin">
                    <span class="file-info" id="fw_info">No file chosen</span>
                </div>
                <div class="progress-container" id="prg_firmware">
                    <div class="progress-label"><span>Firmware</span><span id="pct_firmware">0%</span></div>
                    <div class="progress-bar-bg"><div class="progress-bar-fill" id="bar_firmware"></div></div>
                </div>
            </div>

            <div class="upload-section">
                <label>Filesystem (.bin)</label>
                <div class="file-input-wrapper">
                    <label for="filesystem_file" class="custom-file-btn">Choose File</label>
                    <input type='file' id='filesystem_file' accept=".bin">
                    <span class="file-info" id="fs_info">No file chosen</span>
                </div>
                <div class="progress-container" id="prg_filesystem">
                    <div class="progress-label"><span>Filesystem</span><span id="pct_filesystem">0%</span></div>
                    <div class="progress-bar-bg"><div class="progress-bar-fill" id="bar_filesystem"></div></div>
                </div>
            </div>

            <input type='submit' value='Update' id="update_btn">
        </form>

    </div>
    
    <div class="footer-info" onclick="window.location.href='/boardinfo'" style="margin-top: 20px; font-size: 12px; color: #888; display: flex; justify-content: center; align-items: center; gap: 8px; cursor: pointer;">
        <span class="info-pill chip-pill" id="footer_chipid">-</span>
        <span class="info-pill board-pill" id="footer_board">-</span>
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

        // Fetch board info
        fetch('/info')
            .then(response => response.json())
            .then(data => {
                document.getElementById('chip_size').innerText = formatBytes(data.chip_size);
                document.getElementById('sketch_size').innerText = formatBytes(data.sketch_size);
                document.getElementById('free_sketch_space').innerText = formatBytes(data.free_sketch_space);
                document.getElementById('fs_size').innerText = formatBytes(data.fs_size);
                document.getElementById('fs_used').innerText = formatBytes(data.fs_used) + ' (' + Math.round(data.fs_used/data.fs_size*100) + '%)';
                
                // Draw Partition Bar
                const total = data.chip_size;
                const sketchPct = (data.sketch_size / total) * 100;
                const freePct = (data.free_sketch_space / total) * 100;
                const fsPct = (data.fs_size / total) * 100;
                const otherPct = 100 - (sketchPct + freePct + fsPct);
                
                const bar = document.getElementById('partition_bar');
                bar.innerHTML = `
                    <div class="part-segment part-sketch" style="width: ${sketchPct}%" title="Sketch: ${formatBytes(data.sketch_size)}">App</div>
                    <div class="part-segment part-free" style="width: ${freePct}%" title="Free: ${formatBytes(data.free_sketch_space)}">OTA</div>
                    <div class="part-segment part-fs" style="width: ${fsPct}%" title="FS: ${formatBytes(data.fs_size)}">FS</div>
                    <div class="part-segment part-other" style="width: ${otherPct < 0 ? 0 : otherPct}%" title="Reserved/Other"></div>
                `;
                // Populate footer
                document.getElementById('footer_chipid').innerText = data.chip_id || '-';
                document.getElementById('footer_board').innerText = data.board || '-';
            })
            .catch(err => console.error('Failed to load board info', err));

        // File Input Listeners
        function setupFileInput(inputId, infoId) {
            document.getElementById(inputId).addEventListener('change', function() {
                const file = this.files[0];
                if (file) {
                    document.getElementById(infoId).innerText = `${file.name} (${formatBytes(file.size)})`;
                } else {
                    document.getElementById(infoId).innerText = "No file chosen";
                }
            });
        }
        setupFileInput('firmware_file', 'fw_info');
        setupFileInput('filesystem_file', 'fs_info');

        document.getElementById('upload_form').onsubmit = async function(e) {
            e.preventDefault();
            
            var fwFile = document.getElementById('firmware_file').files[0];
            var fsFile = document.getElementById('filesystem_file').files[0];
            var btn = document.getElementById('update_btn');

            if (!fwFile && !fsFile) {
                alert("Please select at least one file!");
                return;
            }

            btn.disabled = true;
            btn.value = "Updating...";

            try {
                if (fwFile) {
                    await uploadFile(fwFile, 'firmware', 'prg_firmware', 'pct_firmware', 'bar_firmware', !fsFile);
                }
                if (fsFile) {
                    await uploadFile(fsFile, 'filesystem', 'prg_filesystem', 'pct_filesystem', 'bar_filesystem', true);
                }
            } catch (err) {
                alert("Update Error: " + err);
                btn.disabled = false;
                btn.value = "Update";
            }
        };

        function uploadFile(file, type, prgId, pctId, barId, reboot) {
            return new Promise((resolve, reject) => {
                document.getElementById(prgId).style.display = 'block';
                var formData = new FormData();
                formData.append("update", file);
                
                var url = '/update?output=json&name=' + type;
                if (!reboot) {
                    url += '&reboot=false';
                }

                var xhr = new XMLHttpRequest();
                xhr.upload.addEventListener('progress', function(evt) {
                    if (evt.lengthComputable) {
                        var per = Math.round((evt.loaded / evt.total) * 100);
                        document.getElementById(barId).style.width = per + '%';
                        document.getElementById(pctId).innerText = per + '%';
                    }
                }, false);

                xhr.open('POST', url, true);
                xhr.onload = function() {
                    if (xhr.status === 200) {
                        document.getElementById(barId).style.backgroundColor = '#28a745';
                        document.getElementById(pctId).innerText = "Done";
                        if(reboot) {
                            document.querySelector('.container').innerHTML = "<h2>Update Success!</h2><p>Rebooting...</p>";
                            setTimeout(function(){ window.location.href = '/'; }, 5000);
                        }
                        resolve();
                    } else {
                        document.getElementById(barId).style.backgroundColor = '#dc3545';
                        reject(xhr.responseText || "Server Error");
                    }
                };
                xhr.onerror = function() { reject("Network Error"); };
                xhr.send(formData);
            });
        }
    </script>
</body>
</html>
)";

#endif

