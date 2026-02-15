const char settings_html[] PROGMEM = R"=====(
    <div id="page-settings" class="page">
        <!-- System Settings Placeholder -->
        
        <!-- WS Server Status -->
        <div class="card">
            <div class="card-header">
                <h2>📡 WebSocket Server Status</h2>
                <button class="btn btn-sm" onclick="loadWsServerStatus()">↻</button>
            </div>
            <div class="tip-box" style="margin-bottom:15px">
                 Status of the internal WebSocket Server used by this Web Interface and other clients.
            </div>
            
            <div style="display: flex; gap: 20px; flex-wrap: wrap; margin-bottom: 20px;">
                 <div style="flex: 1; padding: 15px; background: rgba(0,0,0,0.03); border-radius: 8px; border-left: 5px solid #28a745;">
                    <div style="color: #666; font-size: 0.9rem;">Status</div>
                    <div style="font-size: 1.2rem; font-weight: bold; margin-top: 5px; color:#28a745">Active</div>
                 </div>
                 <div style="flex: 1; padding: 15px; background: rgba(0,0,0,0.03); border-radius: 8px; border-left: 5px solid #1a73e8;">
                    <div style="color: #666; font-size: 0.9rem;">Clients Connected</div>
                    <div id="ws-server-count" style="font-size: 1.2rem; font-weight: bold; margin-top: 5px;">0</div>
                 </div>
            </div>

            <div class="table-responsive">
                <h4>Recent Traffic (Simulated)</h4>
                 <table id="table-ws-traffic">
                    <thead>
                        <tr>
                            <th>Type</th>
                            <th>Time</th>
                            <th>Client ID</th>
                            <th>Length</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr><td colspan="4" class="empty-state">No recent traffic logged.</td></tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
)=====";
