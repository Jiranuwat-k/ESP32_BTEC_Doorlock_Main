const char index_html[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta
      name="viewport"
      content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no"
    />
    <title>ESP32 Access Control</title>
    <!-- External CSS -->
    <link rel="stylesheet" href="style.css" />
  </head>
  <body>
    <div class="toast-container" id="toast-container"></div>

    <nav>
      <div class="nav-container">
        <a href="#" class="brand" onclick="switchPage('dashboard')"
          ><span style="font-size: 1.5rem">🔐</span> AdminPanel</a
        >
        <ul class="nav-menu">
          <li
            class="nav-item active"
            onclick="switchPage('dashboard')"
            id="nav-dashboard"
          >
            <span>📄</span> Reader Log
          </li>
          <li
            class="nav-item"
            onclick="switchPage('userslog')"
            id="nav-userslog"
          >
            <span>📝</span> User Log
          </li>
          <li class="nav-item" onclick="switchPage('manage')" id="nav-manage">
            <span>👤</span> Members
          </li>
          <li class="nav-item" onclick="switchPage('add')" id="nav-add">
            <span>➕</span> Add/Edit
          </li>
          <li class="nav-item" onclick="switchPage('system')" id="nav-system">
            <span>⚙️</span> System Logs
          </li>
          <li class="nav-item" onclick="switchPage('settings')" id="nav-settings">
            <span>🔧</span> Settings
          </li>
          <li class="nav-item" onclick="switchPage('usage')" id="nav-usage">
            <span>📈</span> Stats
          </li>
          <li class="nav-item" onclick="switchPage('tools')" id="nav-tools">
            <span>🛠️</span> Tools
          </li>
        </ul>
        <div class="profile-menu" id="auth-info" style="display:none">
             <div class="profile-btn" onclick="toggleProfileDropdown()">
                <span id="profile-initials">G</span>
             </div>
             <div class="profile-dropdown" id="profile-dropdown">
                 <div class="profile-avatar-lg" id="profile-avatar-lg">G</div>
                 <div class="profile-name" id="profile-name">Guest</div>
                 <div class="profile-uid" id="profile-uid">Not Logged In</div>
                 <div class="profile-actions">
                    <button id="btn-login-card" onclick="startLoginFlow()" class="btn-logout-google">Login Card</button>
                    <button onclick="showLogoutModal()" class="btn-logout-google">Sign out</button>
                 </div>
             </div>
        </div>
      </div>
    </nav>
    <div class="container">
      <!-- PAGES -->
      <div id="page-dashboard" class="page active">
        <!-- Reader Status -->
        <div class="card">
          <div class="card-header">
             <h2>📡 Reader Status</h2>
             <button class="btn btn-sm" onclick="loadReaderStatus()">↻</button>
          </div>
          <div style="display: flex; gap: 20px; flex-wrap: wrap;">
             <div style="flex: 1; padding: 15px; background: rgba(0,0,0,0.03); border-radius: 8px; border-left: 5px solid var(--primary-color);">
                <div style="color: var(--text-secondary); font-size: 0.9rem;">Reader IN</div>
                <div id="status-in" style="font-size: 1.2rem; font-weight: bold; margin-top: 5px;">Loading...</div>
             </div>
             <div style="flex: 1; padding: 15px; background: rgba(0,0,0,0.03); border-radius: 8px; border-left: 5px solid var(--warning-color);">
                <div style="color: var(--text-secondary); font-size: 0.9rem;">Reader OUT</div>
                <div id="status-out" style="font-size: 1.2rem; font-weight: bold; margin-top: 5px;">Loading...</div>
             </div>
          </div>
        </div>
        
        <!-- Usage Stats (New) -->

        <div class="card">
          <div class="card-header">
            <h2>Recent Access Logs</h2>
            <div>
              <button
                class="btn btn-sm"
                onclick="loadReaderLog()"
                title="Refresh"
              >
                ↻
              </button>
              <button
                class="btn btn-danger btn-sm"
                onclick="clearLog('reader')"
              >
                🗑️ Clear
              </button>
            </div>
          </div>
          <div class="table-responsive">
            <table id="table-reader">
              <thead>
                <tr>
                  <th>Result</th>
                  <th>Time</th>
                  <th>Device</th>
                  <th>UID</th>
                  <th>Role</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td colspan="5" class="empty-state">Loading...</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
      <div id="page-userslog" class="page">
        <div class="card">
          <div class="card-header">
            <h2>History</h2>
            <div>
              <button
                class="btn btn-sm"
                onclick="loadUserLog()"
                title="Refresh"
              >
                ↻
              </button>
              <button class="btn btn-danger btn-sm" onclick="clearLog('users')">
                🗑️ Clear
              </button>
            </div>
          </div>
          <div class="table-responsive">
            <table id="table-userslog">
              <thead>
                <tr>
                  <th>Action</th>
                  <th>Time</th>
                  <th>Target UID</th>
                  <th>By</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td colspan="4" class="empty-state">Loading...</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
      <div id="page-manage" class="page">
        <div class="card">
          <div class="card-header">
            <h2>Members</h2>
            <div style="display: flex; gap: 10px">
              <button class="btn btn-sm" onclick="loadMembers()">
                ↻ Refresh
              </button>
              <button
                class="btn btn-primary btn-sm"
                onclick="clearForm(); switchPage('add')"
              >
                ➕ New
              </button>
            </div>
          </div>
          <div class="table-responsive">
            <table id="table-users">
              <thead>
                <tr>
                  <th>Info</th>
                  <th>Role</th>
                  <th>Access Period (Guest)</th>
                  <th>Actions</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td colspan="4" class="empty-state">Loading...</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <!-- USAGE STATS DETAIL PAGE -->
      <div id="page-usage" class="page">
        <!-- Total Stats Card -->
        <div
          class="card"
          style="
            margin-bottom: 20px;
            background: linear-gradient(135deg, #1a73e8 0%, #0d47a1 100%);
            color: white;
          "
        >
          <div
            class="card-header"
            style="border-bottom: 1px solid rgba(255, 255, 255, 0.2)"
          >
            <h2 style="color: white; margin: 0">📊 Total Access Count</h2>
            <div style="text-align: right">
              <strong style="font-size: 2rem" id="total-access-count">0</strong>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-header">
            <h2>📈 Access Count by User</h2>
            <div>
              <button
                class="btn btn-sm"
                onclick="loadUsageStats()"
                title="Refresh"
              >
                ↻
              </button>
              <button class="btn btn-danger btn-sm" onclick="clearLog('usage')">
                🗑️ Clear
              </button>
            </div>
          </div>
          <div class="table-responsive">
            <table id="table-usage">
              <thead>
                <tr>
                  <th>UID</th>
                  <th>Total Access</th>
                  <th>Last Access</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td colspan="3" class="empty-state">Loading...</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <div id="page-add" class="page">
        <div class="card">
          <div class="card-header">
            <h2 id="form-title">➕ Add New User</h2>
            <button class="btn btn-sm" onclick="clearForm()">↺ Reset</button>
          </div>
          <form onsubmit="return validateForm()">
            <div class="form-group">
              <label>UID / Card ID</label>
              <div style="display: flex; gap: 10px">
                <input
                  type="text"
                  id="uid"
                  name="uid"
                  placeholder="Scan or Type UID"
                  list="datalist-unknown"
                  required
                  autocomplete="off"
                /><button
                  type="button"
                  class="btn btn-sm"
                  onclick="loadUnknownUIDs()"
                  title="Refresh Unknown List"
                >
                  ↻</button
                ><datalist id="datalist-unknown"></datalist>
              </div>
              <div class="tip-box" style="margin-top: 5px">
                Unknown cards from Reader Log appear in the list.
              </div>
            </div>
            <div class="form-grid">
              <div class="form-group">
                <label>Role</label
                ><select id="role" name="role" onchange="toggleGuestFields()">
                  <option value="10">User (Standard)</option>
                  <option value="50">Admin</option>
                  <option value="01">Guest (Time Limited)</option>
                </select>
              </div>
              <div class="form-group">
                <label>Title</label
                ><select id="prefix" name="prefix" required>
                  <option value="10">Mr.</option>
                  <option value="11">Ms.</option>
                  <option value="12">Mrs.</option>
                  <option value="13">Miss</option>
                  <option value="20">Dr.</option>
                  <option value="30">Teacher</option>
                  <option value="40">Student</option>
                  <option value="41">Uni. Student</option>
                  <option value="XX">None</option>
                </select>
              </div>
            </div>
            <div class="form-grid">
              <div class="form-group">
                <label>Name (English)</label
                ><input
                  type="text"
                  id="fname_en"
                  name="fname_en"
                  placeholder="John Doe"
                  required
                />
              </div>
              <div class="form-group">
                <label>Name (Thai)</label
                ><input
                  type="text"
                  id="fname_th"
                  name="fname_th"
                  placeholder="ชื่อ-สกุล"
                  required
                />
              </div>
            </div>
            <div class="form-grid">
              <div class="form-group">
                <label>ID Code</label
                ><input type="text" id="code" name="code" required />
              </div>
              <div class="form-group">
                <label>Gender</label
                ><select id="gender" name="gender" required>
                  <option value="1">Male</option>
                  <option value="2">Female</option>
                  <option value="X">Other</option>
                </select>
              </div>
              <div class="form-group">
                <label>Age</label><input type="number" id="age" name="age" />
              </div>
            </div>
            <div
              id="guest-fields"
              style="
                display: none;
                background: rgba(255, 204, 0, 0.1);
                padding: 15px;
                border-radius: 8px;
                margin-bottom: 15px;
                border: 1px dashed orange;
              "
            >
              <label style="color: #d48806; font-weight: bold"
                >📅 Access Period (Required for Guest)</label
              >
              <div class="form-grid">
                <div class="form-group">
                  <label>Start From</label
                  ><input
                    type="datetime-local"
                    id="start_date"
                    name="start_date"
                  />
                </div>
                <div class="form-group">
                  <label>Expire At</label
                  ><input type="datetime-local" id="end_date" name="end_date" />
                </div>
              </div>
            </div>
            <button
              type="submit"
              class="btn btn-primary btn-full"
              id="btn-save"
            >
              💾 Save User
            </button>
          </form>
        </div>
      </div>
      <div id="page-system" class="page">
        <div class="card">
          <div class="card-header">
            <h2>System Logs</h2>
            <div>
              <button class="btn btn-sm" onclick="loadSystemLog()">↻</button>
              <button
                class="btn btn-danger btn-sm"
                onclick="clearLog('system')"
              >
                🗑️ Clear
              </button>
            </div>
          </div>
          <div class="table-responsive">
            <table id="table-system">
              <thead>
                <tr>
                  <th>Time</th>
                  <th>Code</th>
                  <th>Message</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td colspan="3" class="empty-state">Loading...</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
        
      </div>

    <!-- SETTINGS PLACEHOLDER -->
    %SETTINGS_SECTION%

    <!-- TOOLS PLACEHOLDER -->
    %TOOLS_SECTION%

    </div>

    <!-- FLOATING ACTION BUTTON -->
    <div class="fab-container" id="fab">
      <div class="fab-menu">
        <!-- can use target="_blank" -->
        <a href="/update" class="fab-item">☁️ Update FW</a>
        <a href="/update/boardinfo" class="fab-item">ℹ️ Board Info</a>
        <a href="#" class="fab-item" onclick="exportCurrentTable('csv'); return false;">💾 Export CSV</a>
        <a href="#" class="fab-item" onclick="exportCurrentTable('xls'); return false;">📊 Export Excel</a>
      </div>
      <button
        class="fab-main"
        onclick="document.getElementById('fab').classList.toggle('active')"
      >
        ⚡
      </button>
    </div>

    <!-- AUTH MODAL -->
    <div id="auth-modal" class="modal">
      <div class="modal-content">
        <h2 style="margin-bottom: 10px">Security Check</h2>
        <p style="color: var(--text-secondary)">
          This action requires authorization.<br />Please scan <b>Admin</b> or
          <b>MainKey</b>.
        </p>
        <div class="scan-animation">
          <span style="font-size: 2rem">💳</span>
        </div>
        <p
          id="scan-status"
          style="font-weight: bold; color: var(--primary-color)"
        >
          Waiting...
        </p>
        <button
          class="btn btn-sm"
          onclick="cancelLogin()"
          style="margin-top: 20px; background: #eee; color: #333"
        >
          Cancel
        </button>
      </div>
    </div>

    <!-- LOGOUT MODAL -->
    <div id="logout-modal" class="modal">
      <div class="modal-content">
        <h3 style="margin-top: 0">🚪 Logout Options</h3>
        <p style="color: var(--text-secondary)">Select session to terminate:</p>
        <div
          style="
            display: flex;
            flex-direction: column;
            gap: 12px;
            margin-top: 20px;
          "
        >
          <button
            class="btn"
            style="background: #ff9800; color: white"
            onclick="logoutCard()"
            id="btn-logout-card"
          >
            💳 Sign out Card (MainKey/Admin)
          </button>
          <button class="btn btn-danger" onclick="logoutWeb()">
            🌐 Sign out Web System
          </button>
          <button
            class="btn"
            style="background: #eee; color: #333"
            onclick="closeLogoutModal()"
          >
            Cancel
          </button>
        </div>
      </div>
    </div>

    <!-- GENERIC CONFIRM MODAL -->
    <div id="confirm-modal" class="modal">
      <div class="modal-content" style="max-width: 320px; text-align: center; padding: 25px;">
        <h3 id="confirm-title" style="margin-top:0; font-size:1.3rem;">⚠️ Confirm Action</h3>
        <p id="confirm-message" style="margin: 15px 0; color: var(--text-secondary); font-size:1rem;">Are you sure?</p>
        <div style="display: flex; justify-content: center; gap: 15px; margin-top: 25px;">
           <button class="btn" id="btn-confirm-cancel" style="background:#eee; color:#333; flex:1; border:none;">Cancel</button>
           <button class="btn btn-primary" id="btn-confirm-yes" style="flex:1">Confirm</button>
        </div>
      </div>
    </div>

    <!-- External JS -->
    <script src="script.js"></script>
  </body>
</html>
)=====";
