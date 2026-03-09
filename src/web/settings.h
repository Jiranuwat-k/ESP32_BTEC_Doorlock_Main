const char settings_html[] PROGMEM = R"=====(
    <div id="page-settings" class="page">
        <!-- System Settings -->
        <div class="card">
            <div class="card-header">
                <h2>⚙️ System Settings</h2>
            </div>
            <div class="tip-box" style="margin-bottom:15px">
                 Manage web console credentials and anti-passback features.<br>
                 <b>Note:</b> You must be logged in as an Admin/MainKey to save settings.
            </div>
            <div class="form-grid">
                <div class="form-group">
                    <label>Web Username</label>
                    <input type="text" id="set-username" placeholder="Username">
                </div>
                <div class="form-group">
                    <label>Web Password</label>
                    <input type="password" id="set-password" placeholder="New Password (leave blank to keep current)">
                </div>
            </div>
            <div style="margin-top: 15px;">
                <button class="btn btn-primary" style="padding: 15px; width: 100%; box-shadow: 0 4px 12px rgba(24, 119, 242, 0.15);" onclick="saveSystemSettings()">💾 Update Credentials</button>
            </div>
            
            <hr style="margin: 30px 0; border: 0; border-top: 1px solid rgba(0,0,0,0.05);">

            <div class="form-group">
                <label>Anti-Passback System</label>
                <select id="set-antipassback" style="margin-bottom: 10px;">
                    <option value="true">Enabled (Strict IN/OUT)</option>
                    <option value="false">Disabled (Ignore IN/OUT state)</option>
                </select>
                <div class="tip-box">
                    If enabled, users must tap IN before they can tap OUT.
                </div>
                <div style="margin-top: 20px;">
                    <button class="btn" style="background-color: #673ab7; color: white; padding: 15px; width: 100%; box-shadow: 0 4px 12px rgba(103, 58, 183, 0.2);" onclick="saveAntiPassback()">💾 Update Anti-Passback</button>
                </div>
            </div>
        </div>

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
