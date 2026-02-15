const char style_css[] PROGMEM = R"=====(
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
.nav-menu { display: flex; gap: 5px; list-style: none; margin-left: auto; overflow-x: auto; white-space: nowrap; -webkit-overflow-scrolling: touch; padding-bottom: 5px; } 
.nav-menu::-webkit-scrollbar { height: 4px; }
.nav-menu::-webkit-scrollbar-thumb { background: rgba(0,0,0,0.2); border-radius: 4px; }
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
  transition: all 0.3s ease;
  display: flex;
  flex-direction: column;
  gap: 10px;
  align-items: flex-end;
  margin-bottom: 0;
  height: 0; 
  overflow: hidden;
}
.fab-container:hover .fab-menu,
.fab-container.active .fab-menu {
  opacity: 1;
  pointer-events: auto;
  transform: translateY(0);
  height: auto;
  overflow: visible;
  margin-bottom: 10px;
  padding-bottom: 5px; /* Slight buffer */
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
)=====";
