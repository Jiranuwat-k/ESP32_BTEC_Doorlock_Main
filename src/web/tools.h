const char tools_html[] PROGMEM = R"=====(
    <div id="page-tools" class="page">
        <!-- WS Test Card (Client) -->
        <div class="card">
            <div class="card-header">
                <h2>🛠️ WebSocket Client Settings (Outgoing)</h2>
            </div>
            <div class="tip-box" style="margin-bottom:15px">
                 Connect this ESP32 to an external WebSocket Server (e.g. Node-RED).
            </div>
            <div class="form-grid">
                <div class="form-group">
                    <label>Host (IP or Hostname)</label>
                    <input type="text" id="ws-host" placeholder="192.168.x.x or hostname">
                </div>
                <div class="form-group">
                    <label>Port</label>
                    <input type="number" id="ws-port" placeholder="80" value="80">
                </div>
            </div>
            <div class="form-grid">
                <div class="form-group">
                    <label>Path</label>
                    <input type="text" id="ws-path" placeholder="/ws" value="/ws">
                </div>
                <div class="form-group">
                    <label>Topic / Prefix (use comma ',' for multiple)</label>
                    <input type="text" id="ws-prefix" placeholder="UID, LOG" value="UID">
                </div>
            </div>
            <div style="margin-bottom: 20px;">
                <button class="btn btn-primary" onclick="initWebSocket(true)">🔗 Connect / Reconnect</button>
            </div>

            <div class="form-group" style="margin-bottom:0; border-top: 1px dashed #eee; padding-top: 20px;">
                <label>Simulate Message</label>
                <div style="display:flex; gap:10px;">
                    <input type="text" id="ws-test-uid" placeholder="Message Content (e.g. AABBCC)">
                    <button class="btn btn-primary" onclick="sendWsTest()">Send</button>
                </div>
                <div class="tip-box" style="margin-top:10px">
                    Sends: <span id="ws-preview-msg" style="font-family:monospace; font-weight:bold">UID:AABBCC</span>
                </div>
            </div>
            <div class="tip-box" style="margin-top:15px">
                Status: <span id="ws-status" style="font-weight:bold; color:orange;">Disconnected</span>
            </div>
            
            <div class="form-group" style="margin-bottom:0; border-top: 1px dashed #eee; padding-top: 20px; margin-top: 20px;">
                <label>Reader Remote Control</label>
                <div class="tip-box" style="margin-bottom:10px">
                     Commands are broadcasted to all connected clients (e.g. Reader IN).
                </div>
                <div style="display:flex; gap:10px;">
                    <button class="btn btn-danger btn-sm" onclick="sendRemoteCmd('rst ESP')">⚡ Restart ESP ReaderIN Online</button>
                    <button class="btn btn-warning btn-sm" onclick="sendRemoteCmd('rst rc522')">🔄 Restart ReaderIN Online</button>
                </div>
                <div style="display:flex; gap:10px; margin-top: 10px;">
                    <button class="btn btn-danger btn-sm" onclick="performAction('rst_hardware')">🔴 Reset Remote Reader (IN)</button>
                    <button class="btn btn-warning btn-sm" onclick="performAction('rst_reader_out')">🔄 Reset Local Reader (OUT)</button>
                    <button class="btn btn-primary btn-sm" onclick="performAction('rst_self')">♻️ Restart This Unit</button>
                </div>
            </div>
        </div>

        <!-- Network Scanner -->
        <div class="card">
            <div class="card-header">
                <h2>🔎 Network Scanner (mDNS)</h2>
                <button class="btn btn-sm" onclick="scanNetwork()">↻ Scan Devices</button>
            </div>
            <div class="tip-box" style="margin-bottom:15px">
                 Finds other devices on the network advertising HTTP services (e.g. other ESP32s).
            </div>
            <div class="table-responsive">
                <table id="table-scanner">
                    <thead>
                        <tr>
                            <th>Hostname</th>
                            <th>IP Address</th>
                            <th>MAC / Vendor</th>
                            <th>Port</th>
                            <th>Action</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr><td colspan="5" class="empty-state">Click Scan to start...</td></tr>
                    </tbody>
                </table>
            </div>
        </div>
        <!-- Real-time Access Display -->
        <div class="card" id="card-display" style="border-left: 5px solid #673ab7;">
            <div class="card-header">
                <h2>📺 Real-time Access Display</h2>
            </div>
            <div class="tip-box" style="margin-bottom:15px">
                 Shows live card scans with user details. Useful for a dedicated display screen.
            </div>
            
            <div id="display-content" style="text-align:center; padding: 20px;">
                <div style="font-size: 3rem; margin-bottom: 20px;">💳</div>
                <h3 style="color:#666">Waiting for card...</h3>
            </div>
            
            <div id="display-info" style="display:none; text-align:center;">
                <!-- Avatar -->
                <div style="width:100px; height:100px; background:#673ab7; color:white; border-radius:50%; font-size:40px; line-height:100px; margin:0 auto 15px auto;" id="disp-avatar">
                   U
                </div>
                
                <h2 id="disp-name" style="margin:0; font-size:1.8rem;">John Doe</h2>
                <div id="disp-role" style="color:#666; font-size:1.1rem; margin-bottom:15px;">Student</div>
                
                <div style="display:flex; justify-content:center; gap:20px; margin-top:20px; border-top:1px solid #eee; padding-top:20px;">
                    <div>
                        <div style="font-size:0.9rem; color:#888;">STATUS</div>
                        <div id="disp-status" style="font-size:1.2rem; font-weight:bold; color:green;">ENTERED</div>
                    </div>
                     <div>
                        <div style="font-size:0.9rem; color:#888;">TIME SPENT</div>
                        <div id="disp-duration" style="font-size:1.2rem; font-weight:bold;">-</div>
                    </div>
                </div>
            </div>
        </div>
    </div>
)=====";
