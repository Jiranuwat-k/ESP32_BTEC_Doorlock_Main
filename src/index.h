const char index_html[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta
      name="viewport"
      content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no"
    />
    <title>ESP32 Access Control</title>
    <!-- <link rel="stylesheet" href="style.css" /> -->
    <style>
      :root {
          --bg-color: #f0f2f5;
          --card-bg: #ffffff;
          --text-color: #1c1e21;
          --text-secondary: #65676b;
          --primary-color: #1877f2;
          --primary-hover: #166fe5;
          --danger-color: #ff3b30;
          --success-color: #34c759;
          --warning-color: #ffcc00;
          --border-radius: 12px;
          --shadow: 0 1px 3px rgba(0, 0, 0, 0.1), 0 1px 2px rgba(0, 0, 0, 0.06);
          --font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
          --nav-height: 60px;
      }
      @media (prefers-color-scheme: dark) {
          :root {
              --bg-color: #18191a;
              --card-bg: #242526;
              --text-color: #e4e6eb;
              --text-secondary: #b0b3b8;
              --primary-color: #2d88ff;
              --shadow: 0 1px 3px rgba(0, 0, 0, 0.3);
          }
      }
      * { box-sizing: border-box; margin: 0; padding: 0; }
      body { font-family: var(--font-family); background-color: var(--bg-color); color: var(--text-color); padding-bottom: 80px; }
      nav { background: var(--card-bg); box-shadow: var(--shadow); height: var(--nav-height); position: sticky; top: 0; z-index: 1000; display: flex; align-items: center; padding: 0 1rem; }
      .nav-container { max-width: 1200px; width: 100%; margin: 0 auto; display: flex; align-items: center; gap: 20px; }
      .brand { font-size: 1.25rem; font-weight: 700; color: var(--primary-color); text-decoration: none; display: flex; align-items: center; gap: 8px; }
      .nav-menu { display: flex; gap: 5px; list-style: none; margin-left: auto; }
      .nav-item { cursor: pointer; padding: 0.5rem 1rem; border-radius: 8px; font-weight: 500; font-size: 0.95rem; color: var(--text-secondary); transition: all 0.2s; text-decoration: none; display: flex; align-items: center; gap: 6px; }
      .nav-item:hover { background: rgba(0, 0, 0, 0.05); color: var(--text-color); }
      .nav-item.active { color: var(--primary-color); background: rgba(24, 119, 242, 0.1); }
      .mobile-menu-btn { display: none; background: none; border: none; font-size: 1.5rem; color: var(--text-color); cursor: pointer; }
      @media (max-width: 768px) {
          .nav-menu { position: fixed; bottom: 0; left: 0; right: 0; background: var(--card-bg); justify-content: space-around; padding: 10px 0; box-shadow: 0 -1px 3px rgba(0, 0, 0, 0.1); z-index: 999; }
          .nav-item { flex-direction: column; font-size: 0.75rem; gap: 2px; padding: 5px; flex: 1; text-align: center; border-radius: 0; }
          .nav-item span { display: block; font-size: 1.2rem; margin-bottom: 2px; }
          body { padding-bottom: 80px; }
          .profile-menu { margin-left: auto; }
          .profile-dropdown { position: fixed; top: 60px; right: 10px; width: calc(100% - 20px); max-width: 300px; }
      }
      .page { display: none; animation: fadeIn 0.3s ease; }
      .page.active { display: block; }
      @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }
      .container { max-width: 1200px; margin: 20px auto; padding: 0 15px; }
      .card { background: var(--card-bg); border-radius: var(--border-radius); padding: 20px; box-shadow: var(--shadow); margin-bottom: 20px; }
      .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }
      h2 { font-size: 1.4rem; color: var(--text-color); }
      .table-responsive { overflow-x: auto; -webkit-overflow-scrolling: touch; }
      table { width: 100%; border-collapse: collapse; white-space: nowrap; }
      th, td { padding: 12px 15px; text-align: left; border-bottom: 1px solid rgba(0, 0, 0, 0.05); }
      th { font-weight: 600; color: var(--text-secondary); font-size: 0.85rem; text-transform: uppercase; }
      tr:last-child td { border-bottom: none; }
      .badge { padding: 4px 8px; border-radius: 6px; font-size: 0.8rem; font-weight: 600; }
      .bg-success { background: rgba(52, 199, 89, 0.15); color: #34c759; }
      .bg-danger { background: rgba(255, 59, 48, 0.15); color: #ff3b30; }
      .bg-warning { background: rgba(255, 204, 0, 0.15); color: #d48806; }
      .bg-info { background: rgba(24, 119, 242, 0.15); color: #1877f2; }
      .bg-dark { background: rgba(0, 0, 0, 0.1); color: var(--text-color); }
      .form-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }
      .form-group { margin-bottom: 15px; }
      label { display: block; margin-bottom: 6px; font-size: 0.9rem; font-weight: 500; color: var(--text-secondary); }
      input { width: 100%; padding: 10px; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; background: var(--bg-color); color: var(--text-color); font-size: 0.95rem; outline: none; transition: 0.2s; }
      select { width: 100%; padding: 10px; border: 1px solid rgba(0, 0, 0, 0.1); border-radius: 8px; background: var(--bg-color); color: var(--text-color); font-size: 0.95rem; outline: none; transition: 0.2s;
        appearance: none; -webkit-appearance: none;
        background-image: url("data:image/svg+xml;charset=UTF-8,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%2365676b' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3e%3cpolyline points='6 9 12 15 18 9'%3e%3c/polyline%3e%3c/svg%3e");
        background-repeat: no-repeat; background-position: right 1rem center; background-size: 1em; padding-right: 2.5rem;
        white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 100%; }
      input:focus, select:focus { border-color: var(--primary-color); background: var(--card-bg); box-shadow: 0 0 0 2px rgba(24, 119, 242, 0.2); }
      @media (prefers-color-scheme: dark) { select { background-image: url("data:image/svg+xml;charset=UTF-8,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%23b0b3b8' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3e%3cpolyline points='6 9 12 15 18 9'%3e%3c/polyline%3e%3c/svg%3e"); } }
      input[type="datetime-local"] { font-family: inherit; color: var(--text-color); }
      ::-webkit-calendar-picker-indicator { filter: invert(0.5); cursor: pointer; }
      .btn { display: inline-flex; align-items: center; gap: 6px; justify-content: center; padding: 8px 16px; border: none; border-radius: 8px; font-weight: 600; cursor: pointer; background: var(--bg-color); color: var(--text-color); text-decoration: none; transition: 0.2s; font-size: 0.9rem; }
      .btn-primary { background: var(--primary-color); color: white; }
      .btn-primary:hover { background: var(--primary-hover); }
      .btn-danger { background: rgba(255, 59, 48, 0.1); color: var(--danger-color); }
      .btn-danger:hover { background: rgba(255, 59, 48, 0.2); }
      .btn-sm { padding: 4px 10px; font-size: 0.8rem; }
      .btn-full { width: 100%; padding: 12px; margin-top: 10px; }
      .empty-state { text-align: center; padding: 40px 0; color: var(--text-secondary); }
      .empty-icon { font-size: 3rem; margin-bottom: 10px; display: block; opacity: 0.5; }
      .tip-box { background: rgba(24, 119, 242, 0.05); border: 1px dashed var(--primary-color); border-radius: 8px; padding: 10px; font-size: 0.85rem; color: var(--text-secondary); }
      .empty-icon { font-size: 3rem; margin-bottom: 10px; display: block; opacity: 0.5; }
      .tip-box { background: rgba(24, 119, 242, 0.05); border: 1px dashed var(--primary-color); border-radius: 8px; padding: 10px; font-size: 0.85rem; color: var(--text-secondary); }
      
      /* Google-like Profile */
      .profile-menu { position: relative; }
      .profile-btn { width: 40px; height: 40px; border-radius: 50%; background: var(--primary-color); color: white; display: flex; align-items: center; justify-content: center; font-weight: bold; font-size: 1.2rem; cursor: pointer; border: 2px solid white; box-shadow: 0 2px 5px rgba(0,0,0,0.2); user-select: none; }
      .profile-btn:hover { box-shadow: 0 4px 8px rgba(0,0,0,0.3); }
      .profile-dropdown { position: absolute; top: 50px; right: 0; background: var(--card-bg); border-radius: 16px; box-shadow: 0 5px 20px rgba(0,0,0,0.2); width: 300px; padding: 20px; display: none; flex-direction: column; align-items: center; z-index: 2000; animation: fadePop 0.2s ease; border: 1px solid rgba(0,0,0,0.1); }
      .profile-dropdown.show { display: flex; }
      .profile-avatar-lg { width: 80px; height: 80px; border-radius: 50%; background: var(--primary-color); color: white; display: flex; align-items: center; justify-content: center; font-size: 2.5rem; font-weight: bold; margin-bottom: 10px; }
      .profile-name { font-size: 1.1rem; font-weight: 600; color: var(--text-color); margin-bottom: 5px; }
      .profile-uid { font-size: 0.9rem; color: var(--text-secondary); margin-bottom: 20px; font-family: monospace; background: rgba(0,0,0,0.05); padding: 4px 8px; border-radius: 12px; }
      .profile-actions { width: 100%; border-top: 1px solid rgba(0,0,0,0.1); padding-top: 15px; display: flex; justify-content: center; gap: 10px; }
      .btn-logout-google { border: 1px solid #dadce0; background: transparent; color: #3c4043; padding: 10px 24px; border-radius: 4px; font-weight: 500; cursor: pointer; transition: 0.2s; }
      .btn-logout-google:hover { background: rgba(0,0,0,0.05); color: var(--text-color); }
      @keyframes fadePop { from { opacity: 0; transform: translateY(-10px); } to { opacity: 1; transform: translateY(0); } }

      /* TOAST NOTIFICATION COPY FROM ORIGINAL */
      .toast-container {
        position: fixed;
        bottom: 20px;
        left: 20px;
        z-index: 3000;
        display: flex;
        flex-direction: column;
        gap: 10px;
      }
      /* On mobile, raise it so it doesn't cover the bottom nav */
      @media (max-width: 768px) {
        .toast-container {
             bottom: 90px;
             left: 10px;
        }
      }

      .toast {
        background: white;
        padding: 15px 20px;
        border-radius: 8px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
        display: flex;
        align-items: center;
        gap: 12px;
        animation: slideIn 0.3s ease;
        min-width: 250px;
        border-left: 5px solid #333;
      }
      .toast.success {
        border-left-color: #1a73e8;
      }
      .toast.error {
        border-left-color: #e53935;
      }
      .toast-msg {
        font-size: 0.95rem;
        color: #333;
        font-weight: 500;
      }
      @keyframes slideIn {
        from {
          transform: translateX(-100%);
          opacity: 0;
        }
        to {
          transform: translateX(0);
          opacity: 1;
        }
      }
      @keyframes fadeOut {
        from {
          opacity: 1;
        }
        to {
          opacity: 0;
        }
      }

      /* MODAL STYLES */
      .modal {
        display: none;
        position: fixed;
        z-index: 3000;
        left: 0;
        top: 0;
        width: 100%;
        height: 100%;
        background-color: rgba(0, 0, 0, 0.6);
        align-items: center;
        justify-content: center;
        backdrop-filter: blur(5px);
      }
      .modal-content {
        background-color: var(--card-bg);
        padding: 30px;
        border-radius: 20px;
        text-align: center;
        max-width: 90%;
        width: 380px;
        box-shadow: 0 8px 30px rgba(0, 0, 0, 0.3);
        animation: popIn 0.4s cubic-bezier(0.18, 0.89, 0.32, 1.28);
      }
      @keyframes popIn {
        from {
          transform: scale(0.8);
          opacity: 0;
        }
        to {
          transform: scale(1);
          opacity: 1;
        }
      }
      .scan-animation {
        width: 100px;
        height: 100px;
        background: rgba(26, 115, 232, 0.1);
        border-radius: 50%;
        margin: 25px auto;
        display: flex;
        align-items: center;
        justify-content: center;
        position: relative;
      }
      .scan-animation::after {
        content: "";
        position: absolute;
        width: 100%;
        height: 100%;
        border-radius: 50%;
        border: 2px solid #1a73e8;
        animation: ripple 1.5s infinite;
        opacity: 0;
      }
      @keyframes ripple {
        0% {
          transform: scale(0.8);
          opacity: 1;
        }
        100% {
          transform: scale(1.5);
          opacity: 0;
        }
      }
      .auth-status {
        font-size: 0.85rem;
        padding: 5px 12px;
        background: rgba(0, 0, 0, 0.05);
        border-radius: 20px;
        color: var(--text-secondary);
        font-weight: 600;
      }

      /* FAB Styles */
      .fab-container {
        position: fixed;
        bottom: 30px;
        right: 30px;
        z-index: 2200;
        display: flex;
        flex-direction: column;
        align-items: flex-end;
        gap: 10px;
      }
      @media (max-width: 768px) {
        .fab-container {
           bottom: 100px;
           right: 20px;
        }
      }
      .fab-main {
        width: 56px;
        height: 56px;
        border-radius: 50%;
        background: var(--primary-color);
        border: none;
        box-shadow: 0 4px 15px rgba(26, 115, 232, 0.4);
        color: white;
        font-size: 24px;
        cursor: pointer;
        transition: transform 0.3s, background 0.3s;
        display: flex;
        align-items: center;
        justify-content: center;
      }
      .fab-main:hover {
        transform: scale(1.1);
      }
      .fab-container.active .fab-main {
        transform: rotate(45deg);
        background: var(--danger-color);
        box-shadow: 0 4px 15px rgba(255, 59, 48, 0.4);
      }
      .fab-menu {
        opacity: 0;
        pointer-events: none;
        transform: translateY(20px);
        transition: 0.3s;
        display: flex;
        flex-direction: column;
        gap: 10px;
        align-items: flex-end;
        margin-bottom: 10px;
      }
      .fab-container:hover .fab-menu,
      .fab-container.active .fab-menu {
        opacity: 1;
        pointer-events: auto;
        transform: translateY(0);
      }
      .fab-item {
        background: white;
        color: var(--text-color);
        padding: 10px 18px;
        border-radius: 25px;
        text-decoration: none;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
        font-weight: 600;
        font-size: 0.9rem;
        transition: 0.2s;
        white-space: nowrap;
        display: flex;
        align-items: center;
        gap: 8px;
      }
      .fab-item:hover {
        transform: translateX(-5px);
        color: var(--primary-color);
      }
    </style>
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
            <span>⚙️</span> System
          </li>
          <li class="nav-item" onclick="switchPage('usage')" id="nav-usage">
            <span>📈</span> Stats
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

    <script>
      // TOAST FUNCTION
      function showToast(msg, type = "success") {
        const container = document.getElementById("toast-container");
        const div = document.createElement("div");
        div.className = `toast ${type}`;
        div.innerHTML = `<span style="font-size:1.2rem">${
          type === "success" ? "✅" : "❌"
        }</span><div class="toast-msg">${msg}</div>`;
        container.appendChild(div);
        setTimeout(() => {
          div.style.animation = "fadeOut 0.3s forwards";
          setTimeout(() => div.remove(), 300);
        }, 3000);
      }

      // HELPER: Format UID for display (e.g. AABBCC -> AA BB CC)
      function formatUID(uid) {
        if (!uid || uid.length < 2) return uid;
        if (uid.includes(" ")) return uid;
        return uid.toUpperCase().replace(/(.{2})(?=.)/g, "$1 ");
      }

      let pendingAction = null;
      let pollInterval = null;
      async function postAction(action, params = {}) {
        const formData = new FormData();
        formData.append("action", action);
        for (const k in params) formData.append(k, params[k]);
        try {
          const res = await fetch("/api/action", {
            method: "POST",
            body: formData,
          });
          if (res.status === 401) {
            const txt = await res.text();
            if (txt.includes("Session"))
              window.location.reload(); // Web session expired
            else {
              pendingAction = { action, params };
              startLoginFlow();
            } // Card auth needed
            return;
          }
          if (res.ok) {
            const msg = await res.text();
            showToast(msg, "success"); // Use Toast
            if (action === "save_user") {
              clearForm();
              switchPage("manage");
            } else if (action === "delete_user") loadMembers();
            else {
              const p = document
                .querySelector(".page.active")
                .id.replace("page-", "");
              switchPage(p);
            }
            checkLoginStatus();
          } else {
            showToast(await res.text(), "error");
          } // Use Toast
        } catch (e) {
          console.error(e);
          showToast("Network Error", "error");
        }
      }
      async function startLoginFlow() {
        document.getElementById("auth-modal").style.display = "flex";
        document.getElementById("scan-status").innerText =
          "Starting reader mode...";
        await fetch("/api/login/wait", { method: "POST" });
        document.getElementById("scan-status").innerText =
          "Please tap card now...";
        if (pollInterval) clearInterval(pollInterval);
        pollInterval = setInterval(async () => {
          try {
            const res = await fetch("/api/login/status");
            const data = await res.json();
            if (data.status === "logged_in") {
              clearInterval(pollInterval);
              document.getElementById("auth-modal").style.display = "none";
              updateAuthUI(data.uid);
              if (pendingAction) {
                postAction(pendingAction.action, pendingAction.params);
                pendingAction = null;
              }
            }
          } catch (e) {}
        }, 1000);
      }
      function cancelLogin() {
        document.getElementById("auth-modal").style.display = "none";
        if (pollInterval) clearInterval(pollInterval);
        pendingAction = null;
      }

      // LOGOUT LOGIC
      function showLogoutModal() {
        const uidEl = document.getElementById("profile-uid");
        const statusText = uidEl ? uidEl.innerText : "Not Logged In";

        // Logic: If NOT logged in via card (Not Logged In), Direct Web Logout
        if (statusText === "Not Logged In" || statusText === "") {
             logoutWeb();
             return;
        }

        // If Card Logged In, Show Modal to choose
        document.getElementById("logout-modal").style.display = "flex";
        
        const btnCard = document.getElementById("btn-logout-card");
        if(btnCard) {
             // Reset style just in case
             btnCard.style.opacity = "1";
             btnCard.style.pointerEvents = "auto";
             btnCard.innerText = "💳 Sign out Card (" + statusText + ")";
        }
      }
      function closeLogoutModal() {
        document.getElementById("logout-modal").style.display = "none";
      }
      async function logoutCard() {
        try {
          await fetch("/api/login/logout", { method: "POST" });
          showToast("Card Session Logged Out");
          closeLogoutModal();
          checkLoginStatus();
        } catch (e) {
          showToast("Error logging out card", "error");
        }
      }
      function logoutWeb() {
        window.location.href = "/logout";
      }

      async function checkLoginStatus() {
        try {
          const res = await fetch("/api/login/status");
          const data = await res.json();
          updateAuthUI(data.uid);
        } catch (e) {}
      }
      function toggleProfileDropdown() {
          const dd = document.getElementById("profile-dropdown");
          dd.classList.toggle("show");
      }
      
      // Close dropdown if clicked outside
      window.onclick = function(event) {
          if (!event.target.closest('.profile-btn') && !event.target.closest('.profile-dropdown')) {
             const downs = document.getElementsByClassName("profile-dropdown");
             for (let i = 0; i < downs.length; i++) {
                 if (downs[i].classList.contains('show')) {
                     downs[i].classList.remove('show');
                 }
             }
          }
      }

      function updateAuthUI(uidDisplay) {
        const div = document.getElementById("auth-info");
        
        // Always show
        div.style.display = "block";
        
        let initial = "G";
        let role = "Guest";
        let uid = "Not Logged In";
        
        if (uidDisplay && typeof uidDisplay === "string" && uidDisplay !== "") {
           // Parse "Role (UID)"
           if(uidDisplay.includes("(")) {
               const parts = uidDisplay.split("(");
               role = parts[0].trim();
               uid = parts[1].replace(")", "").trim();
               initial = role.charAt(0).toUpperCase();

               // If Role is Admin, Try to Fetch Name from /view-users
               if(role === "Admin") {
                   fetch("/view-users?r=" + Math.random())
                   .then(r=>r.text())
                   .then(txt => {
                       const lines = txt.split('\n');
                       for(let l of lines) {
                           const c = l.split(',');
                           // Format: UID[0], Role[1], Prefix[2], NameEN[3]
                           if(c.length > 3 && c[0] === uid) {
                               const nameEN = c[3];
                               if(nameEN && nameEN !== "-" && nameEN !== "") {
                                   document.getElementById("profile-name").innerText = nameEN;
                                   document.getElementById("profile-initials").innerText = nameEN.charAt(0).toUpperCase();
                                   document.getElementById("profile-avatar-lg").innerText = nameEN.charAt(0).toUpperCase();
                               }
                           }
                       }
                   });
               }
           } else {
               // Fallback
               role = "User";
               uid = uidDisplay;
               initial = "U";
           }
        }

        document.getElementById("profile-initials").innerText = initial;
        document.getElementById("profile-avatar-lg").innerText = initial;
        document.getElementById("profile-name").innerText = role;
        document.getElementById("profile-uid").innerText = formatUID(uid);

        const btnLogin = document.getElementById("btn-login-card");
        if(btnLogin) {
             if(!uidDisplay || uidDisplay === "") {
                 btnLogin.style.display = "block";
             } else {
                 btnLogin.style.display = "none";
             }
        }
      }
      function switchPage(pageId) {
        document
          .querySelectorAll(".nav-item")
          .forEach((el) => el.classList.remove("active"));
        const navEl = document.getElementById("nav-" + pageId);
        if (navEl) navEl.classList.add("active");
        document
          .querySelectorAll(".page")
          .forEach((el) => el.classList.remove("active"));
        document.getElementById("page-" + pageId).classList.add("active");
        if (pageId === "dashboard") loadReaderLog();
        if (pageId === "userslog") loadUserLog();
        if (pageId === "usage") loadUsageStats();
        if (pageId === "manage") loadMembers();
        if (pageId === "system") loadSystemLog();
        if (pageId === "add") loadUnknownUIDs();
      }
      const roleMap = {
        10: "User",
        50: "Admin",
        "01": "Guest",
        99: "MainKey",
        "00": "Unknown",
      };
      const verifyMap = {
        1: "OK", 2: "DENIED", 3: "NOTFOUND", 4: "EXPIRED", 5: "INVALID",
        7: "LOGIN SUCCESS", 8: "LOGIN DENIED"
      };
      const badgeClass = {
        1: "bg-success", 2: "bg-danger", 3: "bg-warning", 4: "bg-danger", 5: "bg-dark",
        7: "bg-success", 8: "bg-danger"
      };
      const readerMap = { 1: "IN", 2: "OUT" };
      const titleMap = {
        "10": "Mr.", "11": "Ms.", "12": "Mrs.", "13": "Miss", 
        "20": "Dr.", "21": "Prof.", "30": "Tchr.", "40": "Std.", "41": "U.Std."
      };
      const titleMapTH = {
        "10": "นาย", "11": "นางสาว", "12": "นาง", "13": "นางสาว", 
        "20": "ดร.", "21": "ศ.", "30": "ครู", "40": "นักเรียน", "41": "นักศึกษา"
      };

      async function fetchUserMap() {
          try {
              const res = await fetch("view-users?r=" + Math.random());
              const text = await res.text();
              const map = {};
              text.split("\n").slice(1).forEach(r => {
                  const c = r.split(",");
                  if(c.length > 0 && c[0] !== "") map[c[0]] = c;
              });
              return map;
          } catch(e) { return {}; }
      }

      function formatUserInfo(uid, userMap) {
          if(!uid) return "-";
          const c = userMap[uid];
          // If no user details, fallback
          if(!c) return `<div style="font-weight:bold">${formatUID(uid)}</div>`;

          // c: [0:UID, 1:Role, 2:Prefix, 3:EN, 4:TH, 5:Code ...]
          const prefixStr = titleMap[c[2]] || "";
          const prefixStrTH = titleMapTH[c[2]] || "";
          let nameHTML = "";
          
          if (c[4] && c[4] !== "-" && c[4] !== "") nameHTML += `<div>${prefixStrTH}${c[4]}</div>`;
          if (c[3] && c[3] !== "-" && c[3] !== "") nameHTML += `<div style="color:#666; font-size:0.85em">${prefixStr} ${c[3]}</div>`;
          
          // If no names
          if(nameHTML === "") nameHTML = "<div>(No Name)</div>";

          const code = (c[5] && c[5] !== "-" && c[5] !== "") ? 
                       `<div style="font-weight:bold; color:var(--primary-color)">${c[5]}</div>` : "";
          
          return `
            ${code}
            ${nameHTML}
            <small style="font-family:monospace;color:#999; display:block; margin-top:2px">${formatUID(uid)}</small>
          `;
      }

      async function loadReaderLog() {
        try {
          // Parallel Fetch
          const [logRes, userMap] = await Promise.all([
              fetch("view-log?r=" + Math.random()),
              fetchUserMap()
          ]);
          const text = await logRes.text();
          const rows = text.trim().split("\n").slice(1).reverse();
          let html = "";
          rows.forEach((r) => {
            const c = r.split(",");
            if (c.length < 6) return; // Date,Time,Reader,UID,Role,Verify
            const status = verifyMap[c[5]] || "Unknown";
            let badge = badgeClass[c[5]] || "bg-dark";
            
            // Special styling for Login Denied only (optional, or keep standard)
            // if (c[5] == 8) badge = "bg-light text-danger fw-bold border border-danger";

            // Standard Reader Display (IN/OUT)
            let reader = readerMap[c[2]] || c[2];
            
            html += `<tr>
                <td><span class="badge ${badge}">${status}</span></td>
                <td>${c[0]}<br><small style="color:#888">${c[1]}</small></td>
                <td><b>${reader}</b></td>
                <td>${formatUserInfo(c[3], userMap)}</td>
                <td>${roleMap[c[4]] || c[4]}</td>
            </tr>`;
          });
          document.querySelector("#table-reader tbody").innerHTML =
            html || '<tr><td colspan="5" class="empty-state">No logs found</td></tr>';
        } catch (e) {}
      }

      async function loadUserLog() {
        const map = {
          C: '<span class="badge bg-success">Created</span>',
          M: '<span class="badge bg-warning">Modified</span>',
          D: '<span class="badge bg-danger">Deleted</span>',
        };
        try {
          const [logRes, userMap] = await Promise.all([
              fetch("view-userslog?r=" + Math.random()),
              fetchUserMap()
          ]);
          const text = await logRes.text();
          const rows = text.trim().split("\n").slice(1).reverse();
          let html = "";
          rows.forEach((r) => {
            const c = r.split(","); // Date,Time,Event,OperatorUID,TargetUID
            if (c.length < 5) return;
            
            // Operator might be system/admin
            let operator = c[3];
            if(userMap[c[3]]) operator = formatUserInfo(c[3], userMap);
            else if (roleMap[c[3]]) operator = roleMap[c[3]]; // Plain role if no user found
            
            html += `<tr>
                <td>${map[c[2]] || c[2]}</td>
                <td>${c[0]} <small>${c[1]}</small></td>
                <td>${formatUserInfo(c[4], userMap)}</td>
                <td>${operator}</td>
            </tr>`;
          });
          document.querySelector("#table-userslog tbody").innerHTML = html || '<tr><td colspan="4" class="empty-state">No logs found</td></tr>';
        } catch (e) {}
      }

      async function loadSystemLog() {
        try {
          const res = await fetch("view-systemlog?r=" + Math.random());
          const text = await res.text();
          const rows = text.trim().split("\n").slice(1).reverse();
          let html = "";
          rows.forEach((r) => {
            const c = r.split(",");
            if (c.length < 3) return;
            html += `<tr><td>${c[0]} ${c[1]}</td><td><b>${c[2]}</b></td><td>${c[3] || ""}</td></tr>`;
          });
          document.querySelector("#table-system tbody").innerHTML = html || '<tr><td colspan="3" class="empty-state">No logs found</td></tr>';
        } catch (e) {}
      }

      async function loadUsageStats() {
        try {
          const [statRes, userMap] = await Promise.all([
              fetch("view-usage?r=" + Math.random()),
              fetchUserMap()
          ]);
          const text = await statRes.text();
          const rows = text.trim().split("\n").slice(1);

          let html = "";
          let totalStats = 0;

          rows.forEach((r) => {
            const c = r.split(",");
            if (c.length < 3) return;
            const count = parseInt(c[1]) || 0;
            totalStats += count;

            html += `<tr>
                    <td>${formatUserInfo(c[0], userMap)}</td>
                    <td><span class="badge bg-success">${count}</span></td>
                    <td><small>${c[2]}</small></td>
                 </tr>`;
          });

          document.querySelector("#table-usage tbody").innerHTML = html || '<tr><td colspan="3" class="empty-state">No stats found</td></tr>';
          const totalEl = document.getElementById("total-access-count");
          if (totalEl) totalEl.innerText = totalStats;
        } catch (e) {}
      }

      async function loadMembers() {
        try {
          const userMap = await fetchUserMap(); // Use shared fetch
          // But loadMembers needs the array order? fetchUserMap returns obj.
          // Let's refetch as array for members or convert map to array?
          // The order in map keys isn't guaranteed reversed.
          // Better to fetch raw text again for Members to keep order OR just Object.values(map) (but order lost).
          // Let's stick to raw fetch for Members to ensure we match `view-users` original order (or modify fetchUserMap to return list too).
          // Actually, let's just re-fetch to be safe and simple.
          const res = await fetch("view-users?r=" + Math.random());
          const text = await res.text();
          const rows = text.trim().split("\n").slice(1);
          
          let html = "";
          rows.forEach((r) => {
            const c = r.split(",");
            if (c.length < 2) return;
            const inputs = JSON.stringify(c).replace(/'/g, "&apos;");
            
            let guestInfo = '<span class="badge bg-success">Permanent</span>';
            if (c[1] === "01") { // Guest Logic
              const now = new Date();
              const endDate = c[9] ? new Date(c[9]) : null;
              const isExpired = endDate && now > endDate;
              if (isExpired) {
                guestInfo = `<span class="badge bg-danger">EXPIRED</span><br><small>Ended: ${c[9] ? c[9].replace("T", " ") : "-"}</small>`;
              } else {
                guestInfo = `<small>Start: ${c[8] ? c[8].replace("T", " ") : "-"}<br><span style="color:red">End: ${c[9] ? c[9].replace("T", " ") : "-"}</span></small>`;
              }
            }
            
            html += `<tr>
                <td>${formatUserInfo(c[0], userMap)}</td>
                <td>${roleMap[c[1]] || c[1]}</td>
                <td>${guestInfo}</td>
                <td>
                    <button class="btn btn-sm btn-primary" onclick='editUser(${inputs})'>✏️</button> 
                    <button class="btn btn-sm btn-danger" onclick="deleteUser('${c[0]}')">🗑️</button>
                </td>
            </tr>`;
          });
          document.querySelector("#table-users tbody").innerHTML = html || '<tr><td colspan="4" class="empty-state">No members found</td></tr>';
        } catch (e) {}
      }
      async function loadUnknownUIDs() {
        try {
          const uRes = await fetch("view-users?r=" + Math.random());
          const uText = await uRes.text();
          const regSet = new Set();
          uText
            .split("\n")
            .slice(1)
            .forEach((r) => regSet.add(r.split(",")[0]));
          const lRes = await fetch("view-log?r=" + Math.random());
          const lText = await lRes.text();
          const unknownSet = new Set();
          lText.split("\n").forEach((r) => {
            const c = r.split(",");
            if (c.length >= 6 && c[5] === "3" && !regSet.has(c[3]))
              unknownSet.add(c[3]);
          });
          const list = document.getElementById("datalist-unknown");
          list.innerHTML = "";
          Array.from(unknownSet)
            .reverse()
            .forEach((uid) => {
              const opt = document.createElement("option");
              opt.value = uid;
              opt.textContent = "New Scan";
              list.appendChild(opt);
            });
        } catch (e) {}
      }
      function toggleGuestFields() {
        const role = document.getElementById("role").value;
        const div = document.getElementById("guest-fields");
        const start = document.getElementById("start_date");
        const end = document.getElementById("end_date");
        if (role === "01") {
          div.style.display = "block";
          start.required = true;
          end.required = true;
        } else {
          div.style.display = "none";
          start.required = false;
          end.required = false;
        }
      }
      function editUser(data) {
        document.getElementById("uid").value = data[0];
        document.getElementById("uid").readOnly = true;
        document.getElementById("role").value = data[1];
        document.getElementById("prefix").value = data[2] || "XX";
        document.getElementById("fname_en").value = data[3] || "";
        document.getElementById("fname_th").value = data[4] || "";
        document.getElementById("code").value = data[5] || "";
        document.getElementById("gender").value = data[6] || "X";
        document.getElementById("age").value = data[7] || "";
        if (data[8]) document.getElementById("start_date").value = data[8];
        if (data[9]) document.getElementById("end_date").value = data[9];
        toggleGuestFields();
        switchPage("add");
        document.getElementById("form-title").innerText = "✏️ Edit User";
        document.getElementById("btn-save").innerHTML = "💾 Update Changes";
      }
      function clearForm() {
        document.querySelector("form").reset();
        document.getElementById("uid").value = "";
        document.getElementById("uid").readOnly = false;
        document.getElementById("form-title").innerText = "➕ Add New User";
        document.getElementById("btn-save").innerHTML = "💾 Save User";
        toggleGuestFields();
      }

      let confirmCallback = null;
      function showConfirm(msg, callback) {
          document.getElementById("confirm-message").innerText = msg;
          document.getElementById("confirm-modal").style.display = "flex";
          confirmCallback = callback;
      }
      function closeConfirmModal() {
          document.getElementById("confirm-modal").style.display = "none";
          confirmCallback = null;
      }
      // Attach listener once
      document.getElementById("btn-confirm-yes").onclick = function() {
          if(confirmCallback) confirmCallback();
          closeConfirmModal();
      };
      document.getElementById("btn-confirm-cancel").onclick = closeConfirmModal;


      function exportCurrentTable(format = 'csv') {
         const activePage = document.querySelector('.page.active');
         if(!activePage) return;
         const table = activePage.querySelector('table');
         if(!table || table.querySelector('.empty-state')) {
             showToast("No data to export", "error");
             return;
         }
         
         const timestamp = new Date().toISOString().slice(0,19).replace(/[-T:]/g,"");
         let filename = "export_" + activePage.id.replace("page-","") + "_" + timestamp;
         let blobContent = null;
         let mimeType = "";

         if (format === 'csv') {
             const rows = Array.from(table.querySelectorAll('tr'));
             const csvContent = rows.map(row => {
                 const cols = Array.from(row.querySelectorAll('th, td'));
                 // Skip Actions column if present
                 if(cols.length > 0 && cols[cols.length-1].innerText === "Actions") return null;
                 
                 return cols.map((col, index) => {
                     let text = col.innerText.replace(/(\r\n|\n|\r)/gm, " ").replace(/"/g, '""');
                     return `"${text}"`;
                 }).join(",");
             }).filter(r => r).join("\n");
             
             blobContent = "\uFEFF" + csvContent;
             mimeType = 'text/csv;charset=utf-8;';
             filename += ".csv";
         } 
         else if (format === 'xls') {
             // HTML Export for "Excel"
             const clone = table.cloneNode(true);
             
             // Remove Action Columns from Clone
             const rows = Array.from(clone.querySelectorAll('tr'));
             rows.forEach(r => {
                 const cells = Array.from(r.children);
                 if (cells.length > 0) {
                    const lastCell = cells[cells.length-1];
                    if(lastCell.innerText === "Actions" || lastCell.innerHTML.includes("<button")) {
                        r.removeChild(lastCell);
                    }
                 }
                 // Add simple formatting for Excel
                 Array.from(r.children).forEach(td => {
                    td.style.border = "1px solid #ddd";
                    td.style.padding = "5px";
                 });
             });

             const template = `
<html xmlns:o="urn:schemas-microsoft-com:office:office" xmlns:x="urn:schemas-microsoft-com:office:excel" xmlns="http://www.w3.org/TR/REC-html40">
<head>
<!--[if gte mso 9]>
<xml>
<x:ExcelWorkbook>
<x:ExcelWorksheets>
<x:ExcelWorksheet>
<x:Name>Export</x:Name>
<x:WorksheetOptions>
<x:DisplayGridlines/>
</x:WorksheetOptions>
</x:ExcelWorksheet>
</x:ExcelWorksheets>
</x:ExcelWorkbook>
</xml>
<![endif]-->
<meta charset="UTF-8">
<style>table { border-collapse: collapse; } td, th { border: 1px solid #ddd; padding: 5px; mso-number-format:"\@"; }</style>
</head>
<body>
${clone.outerHTML}
</body>
</html>`;
             blobContent = template;
             mimeType = 'application/vnd.ms-excel';
             filename += ".xls";
         }

         const blob = new Blob([blobContent], { type: mimeType });
         const link = document.createElement("a");
         const url = URL.createObjectURL(blob);
         
         link.setAttribute("href", url);
         link.setAttribute("download", filename);
         link.style.visibility = 'hidden';
         document.body.appendChild(link);
         link.click();
         document.body.removeChild(link);
         
         document.getElementById('fab').classList.remove('active'); // Close menu
         showToast("Exported " + filename);
      }

      // Use Toast Confirm for dangerous actions? Yes, now custom modal.
      function clearLog(type) {
        showConfirm("Are you sure you want to clear " + type + " logs?", function() {
           postAction("clear_log", { type: type });
        });
      }
      function deleteUser(uid) {
        showConfirm("Delete User " + uid + "?", function() {
           postAction("delete_user", { uid: uid });
        });
      }
      function validateForm() {
        const form = document.querySelector("form");
        const data = {
          uid: document.getElementById("uid").value,
          role: document.getElementById("role").value,
          prefix: document.getElementById("prefix").value,
          fname_en: document.getElementById("fname_en").value,
          fname_th: document.getElementById("fname_th").value,
          code: document.getElementById("code").value,
          gender: document.getElementById("gender").value,
          age: document.getElementById("age").value,
          start_date: document.getElementById("start_date").value,
          end_date: document.getElementById("end_date").value,
        };
        postAction("save_user", data);
        return false;
      }
      const params = new URLSearchParams(window.location.search);
      if (params.has("uid")) {
        switchPage("add");
        document.getElementById("uid").value = params.get("uid");
      } else {
        switchPage("dashboard");
        checkLoginStatus();
      }
    </script>
  </body>
</html>
)=====";
