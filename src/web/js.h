const char script_js[] PROGMEM = R"=====(
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
      let currentUserIsSuper = false;
      let isElevatedSession = false;
      async function postAction(action, params = {}) {
        const formData = new FormData();
        formData.append("action", action);
        for (const k in params) formData.append(k, params[k]);
        
        const btnSave = document.getElementById("btn-save");
        const originalBtnHtml = btnSave ? btnSave.innerHTML : "";

        try {
          if(action === "save_user" && btnSave) {
              btnSave.disabled = true;
              btnSave.innerHTML = "Processing...";
          }

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
              // Check if we need Super MainKey
              let needsSuper = false;
              if (action === "save_user" && (params.role === "50" || params.role === "90" || params.role === "99")) {
                  needsSuper = true;
              }
              startLoginFlow(needsSuper);
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
        } finally {
            if(action === "save_user" && btnSave) {
                btnSave.disabled = false;
                btnSave.innerHTML = originalBtnHtml;
            }
        }
      }
      async function startLoginFlow(isRestricted = false) {
        document.getElementById("auth-modal").style.display = "flex";
        
        const msgEl = document.getElementById("auth-help-msg");
        if(msgEl) {
             if(isRestricted) {
                 msgEl.innerHTML = "This action requires <b>MainKey</b> privileges.<br>Please scan your <b>MainKey</b>.";
             } else {
                 msgEl.innerHTML = "This action requires authorization.<br>Please scan <b>Admin</b> or <b>MainKey</b>.";
             }
        }

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
              updateAuthUI(data.uid, data.is_super);
              if (pendingAction) {
                if (pendingAction.action === "_custom_save_settings" || pendingAction.action === "_custom_save_apb") {
                      // Re-submit logic:
                      const actionMap = {
                          "_custom_save_settings": saveSystemSettings,
                          "_custom_save_apb": saveAntiPassback
                      };
                      actionMap[pendingAction.action]();
                } else {
                      postAction(pendingAction.action, pendingAction.params);
                }
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
           if (res.status === 401) {
             window.location.href = "/login";
             return;
           }
           const data = await res.json();
           currentUserIsSuper = data.is_super || false;
           updateAuthUI(data.uid, currentUserIsSuper);
         } catch (e) {
             // If fetch fails, it might be a temporary network issue or device rebooting
             console.log("Session check failed, possible reboot...");
         }
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

      function updateAuthUI(uidDisplay, isSuper = false) {
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

         // Access Control for User Enrollment
         const roleSelect = document.getElementById("role");
         const btnSave = document.getElementById("btn-save");
         isElevatedSession = (uidDisplay && uidDisplay !== ""); // Either Card Scanned or Developer Logged In
         
         if(roleSelect) {
              const options = roleSelect.options;
              for(let i=0; i<options.length; i++) {
                  const val = options[i].value;
                  
                  if (!isElevatedSession) {
                      // Case 1: No Admin access at all
                      options[i].disabled = true;
                  } 
                  else if (!isSuper && (val === "50" || val === "90" || val === "99")) {
                      // Case 2: Admin/SecondaryKey elevation but not MainKey
                      options[i].disabled = true;
                  } 
                  else {
                      // Case 3: Proper Permission
                      options[i].disabled = false;
                  }
              }
         }

         if(btnSave) {
             btnSave.disabled = !isElevatedSession;
             btnSave.style.opacity = isElevatedSession ? "1" : "0.5";
             btnSave.style.cursor = isElevatedSession ? "pointer" : "not-allowed";
         }

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
        // Save to localStorage
        localStorage.setItem("activePage", pageId);

        document
          .querySelectorAll(".nav-item")
          .forEach((el) => el.classList.remove("active"));
        const navEl = document.getElementById("nav-" + pageId);
        if (navEl) navEl.classList.add("active");
        document
          .querySelectorAll(".page")
          .forEach((el) => el.classList.remove("active"));
        document.getElementById("page-" + pageId).classList.add("active");
        if (pageId === "dashboard") { loadReaderLog(); loadReaderStatus(); }
        if (pageId === "userslog") loadUserLog();
        if (pageId === "usage") loadUsageStats();
        if (pageId === "manage") loadMembers();
        if (pageId === "system") loadSystemLog();
        if (pageId === "settings") loadWsServerStatus();
        if (pageId === "add") {
            loadUnknownUIDs();
            if (!isElevatedSession) {
                setTimeout(() => startLoginFlow(), 500); // Slight delay for page transition
            }
        }
      }

      function toggleSidebar() {
          const sb = document.getElementById('sidebar');
          const ov = document.getElementById('sidebar-overlay');
          if(sb) sb.classList.toggle('open');
          if(ov) ov.classList.toggle('open');
      }

      function toggleSidebarIfMobile() {
          if(window.innerWidth <= 768) {
              const sb = document.getElementById('sidebar');
              const ov = document.getElementById('sidebar-overlay');
              if(sb) sb.classList.remove('open');
              if(ov) ov.classList.remove('open');
          }
      }

      const roleMap = {
        10: "User",
        50: "Admin",
        "01": "Guest",
        99: "MainKey",
        90: "SecondaryKey",
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

      async function loadReaderStatus() {
        try {
           const res = await fetch("/api/readers");
           const data = await res.json();
           document.getElementById("status-in").innerText = data.in;
           document.getElementById("status-out").innerText = data.out;
        } catch(e) {
           document.getElementById("status-in").innerText = "Error";
           document.getElementById("status-out").innerText = "Error";
        }
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
        const pMap = { 
          "10": {en:"Mr.", th:"นาย"}, "11": {en:"Ms.", th:"นางสาว"}, "12": {en:"Mrs.", th:"นาง"}, 
          "13": {en:"Miss", th:"นางสาว"}, "20": {en:"Dr.", th:"ดร."}, "30": {en:"Teacher", th:"อาจารย์"}, 
          "40": {en:"Student", th:"นักเรียน"}, "41": {en:"Uni. Student", th:"นักศึกษา"}, "XX": {en:"", th:""} 
        };
        const gMap = { "1":"Male", "2":"Female", "X":"Other" };
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
            // Operator styling
            let op = (c[3] || "").replace(/\(|\)/g, "");
            let opBadge = op.replace("DEVELOPER", '<span class="badge" style="background:#5b21b6; color:white">DEV</span>')
                            .replace("MainKey", '<span class="badge" style="background:#6366f1; color:white">MAINKEY</span>');
            if(opBadge === op) opBadge = `<span class="badge" style="background:#3b82f6; color:white">ADMIN</span>`;

            // Snapshot Parsing
            let snap = c[5] || "-";
            let nameEN_disp = snap, nameTH_disp = "", subInfoHtml = "";
            if(snap.includes("[")) {
                try {
                    const mTitle = snap.match(/\[(.*?)\]/);
                    const pEntry = mTitle ? pMap[mTitle[1]] : null;
                    const preEN = pEntry ? pEntry.en : "";
                    const preTH = pEntry ? pEntry.th : "";
                    
                    const mNames = snap.match(/\] (.*?) \((.*?)\)/);
                    const nameEN = mNames ? mNames[1] : "";
                    const nameTH = mNames ? mNames[2] : "";
                    const mID = snap.match(/ID:(.*?) Age/);
                    const idVal = mID ? mID[1].trim() : "";
                    const mAge = snap.match(/Age:(.*?) Sex/);
                    const ageVal = mAge ? mAge[1].trim() : "";
                    const mSex = snap.match(/Sex:(.*?)$/);
                    const sexCode = mSex ? mSex[1].trim() : "";
                    const sexVal = gMap[sexCode] || sexCode;

                    nameEN_disp = (preEN ? preEN + " " : "") + nameEN;
                    nameTH_disp = (preTH ? preTH : "") + nameTH;
                    
                    subInfoHtml = `
                        <span class="badge" style="background:#f8fafc; color:#475569; border:1px solid #e2e8f0; font-size:0.75rem; font-weight:600">ID: ${idVal}</span>
                        <span class="badge" style="background:#f8fafc; color:#475569; border:1px solid #e2e8f0; font-size:0.75rem; font-weight:600">AGE: ${ageVal}</span>
                        <span class="badge" style="background:#f8fafc; color:#475569; border:1px solid #e2e8f0; font-size:0.75rem; font-weight:600">${sexVal.toUpperCase()}</span>
                    `;
                } catch(err) {}
            }

            // Details
            let det = c[6] || "-";
            let detHtml = det;
            if(det.includes("Changed:")) {
                const fields = det.replace("Changed:", "").trim().split(" ");
                detHtml = `<div style="margin-bottom:4px; font-size:0.75rem; color:#64748b; font-weight:700">CHANGES:</div>`;
                fields.forEach(f => { if(f) detHtml += `<span class="badge" style="background:#f1f5f9; color:#334155; border:1px solid #e2e8f0; margin-right:4px; margin-bottom:4px; font-size:0.7rem; font-weight:700;">${f.toUpperCase()}</span>`; });
            } else if (det === "Initial Create") detHtml = `<div style="color:#059669; font-weight:700; font-size:0.85rem">NEW ENROLLMENT</div>`;
            else if (det === "User Deleted") detHtml = `<div style="color:#dc2626; font-weight:700; font-size:0.85rem">REMOVED</div>`;
            
            html += `<tr>
                <td>${map[c[2]] || c[2]}</td>
                <td style="white-space:nowrap; font-weight:600">${c[1]}<br><small style="color:#64748b">${c[0]}</small></td>
                <td>
                   <div style="font-weight:700; font-size:1.05rem; color:#0f172a">${nameEN_disp}</div>
                   <div style="font-size:0.9rem; color:#64748b; margin-bottom:4px">${nameTH_disp}</div>
                   <div style="display:flex; align-items:center; gap:6px; flex-wrap:wrap; margin-top:4px">
                       ${subInfoHtml}
                       <span style="color:#94a3b8; font-size:0.75rem; font-family:monospace; margin-left:4px">${formatUID(c[4])}</span>
                   </div>
                </td>
                <td>${opBadge}</td>
                <td style="max-width:200px; vertical-align:top; border-left:none; padding-left:12px">${detHtml}</td>
            </tr>`;
          });
           document.querySelector("#table-userslog tbody").innerHTML = html || '<tr><td colspan="5" class="empty-state">No logs found</td></tr>';
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

      async function loadSystemSettings() {
          try {
              const res = await fetch("/api/settings/get");
              if (!res.ok) return;
              const data = await res.json();
              document.getElementById("set-username").value = data.sys_username || "";
              document.getElementById("set-password").value = "";
              document.getElementById("set-antipassback").value = data.sys_antipassback ? "true" : "false";
          } catch(e) {}
      }

      async function saveSystemSettings() {
          const user = document.getElementById("set-username").value;
          const pass = document.getElementById("set-password").value;
          
          if (!user) { showToast("Username cannot be empty", "error"); return; }
          
          const formData = new FormData();
          formData.append("sys_username", user);
          if (pass) formData.append("sys_password", pass);

          try {
              const res = await fetch("/api/settings/save", { method: "POST", body: formData });
              if (res.ok) {
                  showToast("Login Credentials Updated", "success");
              } else if (res.status === 401) {
                  pendingAction = { action: "_custom_save_settings", params: formData };
                  startLoginFlow();
              } else {
                  showToast(await res.text(), "error");
              }
          } catch(e) {
              showToast("Network Error", "error");
          }
      }

      async function saveAntiPassback() {
          const apb = document.getElementById("set-antipassback").value;
          const formData = new FormData();
          formData.append("sys_antipassback", apb);

          try {
              const res = await fetch("/api/settings/save", { method: "POST", body: formData });
              if (res.ok) {
                  showToast("Anti-Passback Setting Updated", "success");
              } else if (res.status === 401) {
                  pendingAction = { action: "_custom_save_apb", params: formData };
                  startLoginFlow();
              } else {
                  showToast(await res.text(), "error");
              }
          } catch(e) {
              showToast("Network Error", "error");
          }
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
            
             let actions = "";
             // Role logic: SecondaryKey (90) and MainKey (99) and Admin (50) protected
             const isSensitiveRole = (c[1] === "90" || c[1] === "99" || c[1] === "50");
             
             if (!isSensitiveRole || currentUserIsSuper) {
                 actions = `
                    <button class="btn btn-sm btn-primary" onclick='editUser(${inputs})'>✏️</button> 
                    <button class="btn btn-sm btn-danger" onclick="deleteUser('${c[0]}')">🗑️</button>`;
             } else {
                 actions = `<span class="badge bg-dark">Protected</span>`;
             }
             
             html += `<tr>
                 <td>${formatUserInfo(c[0], userMap)}</td>
                 <td>${roleMap[c[1]] || c[1]}</td>
                 <td>${guestInfo}</td>
                 <td>${actions}</td>
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
      async function validateForm() {
        const uidInput = document.getElementById("uid");
        const uid = uidInput.value.trim();
        if(!uid) { showToast("UID is required", "error"); return; }

        const data = {
          uid: uid,
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

        // If in "Add" mode (UID is editable), check for existing user
        if(!uidInput.readOnly) {
            try {
                const userMap = await fetchUserMap();
                if(userMap[uid]) {
                    showConfirm(`Warning: UID ${uid} is already registered to ${userMap[uid].nameEN}. Overwrite this user?`, function() {
                        postAction("save_user", data);
                    });
                    return;
                }
            } catch(e) {}
        }

        postAction("save_user", data);
      }
      const params = new URLSearchParams(window.location.search);
      const savedPage = localStorage.getItem("activePage");
      if (params.has("uid")) {
        switchPage("add");
        document.getElementById("uid").value = params.get("uid");
      } else if (savedPage) {
        switchPage(savedPage);
        checkLoginStatus();
      } else {
        switchPage("dashboard");
        checkLoginStatus();
      }

      // WebSocket Client for Testing
      var socket;
      function initWebSocket(manual = false) {
        // Load settings from storage or defaults
        let host = localStorage.getItem("ws-host") || window.location.hostname;
        let port = localStorage.getItem("ws-port") || (window.location.port ? window.location.port : "80");
        let path = localStorage.getItem("ws-path") || "/ws";
        let prefix = localStorage.getItem("ws-prefix") || "UID";
        
        // If manual override (user clicked connect), read from inputs
        if (manual) {
             const hInput = document.getElementById("ws-host").value;
             const pInput = document.getElementById("ws-port").value;
             const pathInput = document.getElementById("ws-path").value;
             const prefixInput = document.getElementById("ws-prefix").value;
             
             if(hInput) host = hInput;
             if(pInput) port = pInput;
             if(pathInput) path = pathInput;
             if(prefixInput) prefix = prefixInput;
             
             // Save
             localStorage.setItem("ws-host", host);
             localStorage.setItem("ws-port", port);
             localStorage.setItem("ws-path", path);
             localStorage.setItem("ws-prefix", prefix);
        } else {
             // Fill inputs
             const hEl = document.getElementById("ws-host");
             const pEl = document.getElementById("ws-port");
             const pathEl = document.getElementById("ws-path");
             const prefixEl = document.getElementById("ws-prefix");
             
             if(hEl) hEl.value = host;
             if(pEl) pEl.value = port;
             if(pathEl) pathEl.value = path;
             if(prefixEl) prefixEl.value = prefix;
        }

        // Close existing
        if(socket) {
            socket.onclose = null; // Prevent reconnect loop from old socket
            socket.close();
        }

        // Handle case where host is empty (e.g. file://)
        if(!host) host = "192.168.4.1"; 

        const wsUrl = "ws://" + host + ":" + port + path;
        console.log("Connecting to " + wsUrl);

        try {
            socket = new WebSocket(wsUrl);
            socket.onopen = function() {
                const el = document.getElementById("ws-status");
                if(el) { el.innerText = "Connected (" + wsUrl + ")"; el.style.color = "green"; }
                showToast("WS Connected");
            };
            socket.onmessage = function(e) {
                // Log RX
                logWsTraffic("RX 📥", e.data);
                
                // Handle Access Display
                try {
                    if(e.data.startsWith("{")) {
                        const evt = JSON.parse(e.data);
                        if(evt.type === 'scan_event') {
                            const dispInfo = document.getElementById("display-info");
                            const dispWait = document.getElementById("display-content");
                            if(dispInfo && dispWait) {
                                dispWait.style.display = 'none';
                                dispInfo.style.display = 'block';
                                
                                document.getElementById("disp-name").innerText = evt.name || "Unknown";
                                document.getElementById("disp-role").innerText = (evt.role == "50" ? "Admin" : (evt.role=="10"?"User":"Guest"));
                                document.getElementById("disp-avatar").innerText = (evt.name ? evt.name.charAt(0) : "U");
                                
                                const st = document.getElementById("disp-status");
                                if(evt.reader == 1) { // IN
                                    st.innerText = "ENTERED";
                                    st.style.color = "green";
                                    document.getElementById("disp-duration").innerText = "-";
                                } else { // OUT
                                    st.innerText = "EXITED";
                                    st.style.color = "#ff9800";
                                    
                                    // Format Duration
                                    let dur = evt.duration || 0;
                                    let dStr = "";
                                    if(dur < 60) dStr = dur + " sec";
                                    else if(dur < 3600) dStr = Math.floor(dur/60) + " min";
                                    else dStr = (dur/3600).toFixed(1) + " hr";
                                    
                                    document.getElementById("disp-duration").innerText = dStr;
                                }
                                
                                // Auto reset after 10s?
                                clearTimeout(window.dispTimeout);
                                window.dispTimeout = setTimeout(() => {
                                    dispInfo.style.display = 'none';
                                    dispWait.style.display = 'block';
                                }, 10000);
                            }
                        } else if(evt.type === 'scan_deny') {
                             // Optional: Show Denied Screen
                        } else if(evt.type === 'reader_status') {
                            const status = evt.status;
                            const readerId = evt.reader;
                            const name = (readerId === 1) ? "Reader IN" : "Reader OUT";
                            
                            if(status === "online") {
                                showToast(name + " Connected!", "success");
                            } else {
                                showToast(name + " Disconnected!", "error");
                            }
                            
                            // Update Status Text on Dashboard
                            const elId = (readerId === 1) ? "status-in" : "status-out";
                            const el = document.getElementById(elId);
                            if(el) {
                                // Keep version hex display if available? Only update if we want text.
                                // For now, just set text "Online" / "Offline" or let refresh handle hex.
                                // Dashboard usually shows Ver Hex (e.g. 0x92).
                                // If offline, show "Offline".
                                if(status === "offline") el.innerText = "Offline";
                                else el.innerText = "Online (Verifying...)"; // Will be updated by next poll or we can send Ver?
                                
                                // Trigger a reload of reader status to get actual version
                                loadReaderStatus();
                            }
                        }
                    }
                } catch(err) { console.error("WS Parse", err); }
            };
            socket.onerror = function(e) {
                const el = document.getElementById("ws-status");
                if(el) { el.innerText = "Error"; el.style.color = "red"; }
            };
            socket.onclose = function() {
                const el = document.getElementById("ws-status");
                if(el) { el.innerText = "Disconnected"; el.style.color = "orange"; }
            };
        } catch(e) {
             console.error(e);
             showToast("WS Error: " + e.message, "error");
        }
        
        // Update Preview
        updateWsPreview();
      }
      
      // Helper to log traffic to Settings table
      function logWsTraffic(type, data) {
         const tbody = document.querySelector("#table-ws-traffic tbody");
         if(!tbody) return;
         
         const now = new Date();
         const timeStr = now.getHours() + ":" + now.getMinutes() + ":" + now.getSeconds();
         
         const row = document.createElement("tr");
         row.innerHTML = `
            <td><span class="badge ${type.includes("TX") ? 'badge-primary':'badge-success'}">${type}</span></td>
            <td>${timeStr}</td>
            <td>Self</td>
            <td style="font-family:monospace; font-size:0.9em">${data.length > 20 ? data.substring(0,20)+'...' : data}</td>
         `;
         
         // Remove empty state if exists
         if(tbody.querySelector(".empty-state")) tbody.innerHTML = "";
         
         // Prepend
         tbody.insertBefore(row, tbody.firstChild);
         
         // Limit 10 rows
         while(tbody.rows.length > 10) tbody.removeChild(tbody.lastChild);
      }
      
      function updateWsPreview() {
         const preRaw = document.getElementById("ws-prefix").value;
         const valRaw = document.getElementById("ws-test-uid").value;
         const el = document.getElementById("ws-preview-msg");
         if(el) {
             const prefixes = preRaw.split(',').map(s=>s.trim()).filter(s=>s!=="");
             // Only split values if we have multiple prefixes
             let values = [valRaw];
             if(prefixes.length > 1 && valRaw.includes(',')) {
                 const splitVals = valRaw.split(',').map(s=>s.trim());
                 if(splitVals.length === prefixes.length) {
                     values = splitVals;
                 }
             }

             let list = "";
             if(values.length === prefixes.length) {
                 // 1:1 Mapping
                 list = prefixes.map((p, i) => p + ":" + values[i]).join("; ");
             } else {
                 // Broadcast
                 list = prefixes.map(p => p + ":" + valRaw).join("; ");
             }
             el.innerText = list;
         }
      }
      
      // Listen to inputs for preview and enter key
      const wsP = document.getElementById("ws-prefix");
      const wsT = document.getElementById("ws-test-uid");
      if(wsP) wsP.addEventListener("input", updateWsPreview);
      if(wsT) {
           wsT.addEventListener("input", updateWsPreview);
           wsT.addEventListener("keyup", function(e) {
               if(e.key === "Enter") sendWsTest();
           });
      }

      function sendWsTest() {
          var uid = document.getElementById("ws-test-uid").value;
          var prefixRaw = document.getElementById("ws-prefix").value;
          
          if(uid && socket && socket.readyState === WebSocket.OPEN) {
              const prefixes = prefixRaw.split(',').map(s=>s.trim()).filter(s=>s!=="");
              
              let values = [uid];
              // Check for 1:1 mapping
              if(prefixes.length > 1 && uid.includes(',')) {
                   const splitVals = uid.split(',').map(s=>s.trim());
                   if(splitVals.length === prefixes.length) {
                       values = splitVals;
                   }
              }

              let sentCount = 0;
              if (values.length === prefixes.length) {
                  // Map 1:1
                   prefixes.forEach((p, i) => {
                      const msg = p + ":" + values[i];
                      socket.send(msg);
                      logWsTraffic("TX 📤", msg);
                      sentCount++;
                   });
              } else {
                  // Broadcast
                  prefixes.forEach(p => {
                      const msg = p + ":" + uid;
                      socket.send(msg);
                      logWsTraffic("TX 📤", msg);
                      sentCount++;
                  });
              }
              
              if(sentCount > 0) showToast("Sent " + sentCount + " messages");
              else showToast("No valid topic/prefix", "error");
              
          } else {
              showToast("WS Not Connected", "error");
          }
      }
      
      async function sendRemoteCmd(cmd) {
         if(!confirm("Are you sure you want to send: " + cmd + " to all connected readers?")) return;
         try {
             const res = await fetch("/api/ws/broadcast", {
                 method: "POST",
                 body: new URLSearchParams({msg: cmd})
             });
             if(res.ok) showToast("Command Sent: " + cmd);
             else showToast("Failed to send", "error");
         } catch(e) { showToast("Network Error", "error"); }
      }
      
      async function scanNetwork() {
         const tbody = document.querySelector("#table-scanner tbody");
         tbody.innerHTML = '<tr><td colspan="4" class="empty-state">Starting scan... (Please wait ~10s)</td></tr>';
         
         try {
             // 1. Start Scan
             const startRes = await fetch("/api/scan/start", { method: "POST" });
             if(!startRes.ok) throw new Error("Could not start scan");
             
             // 2. Poll Status
             const pollInterval = setInterval(async () => {
                 try {
                     const statusRes = await fetch("/api/scan/status");
                     const statusData = await statusRes.json();
                     
                     if (statusData.scanning) {
                         tbody.innerHTML = `<tr><td colspan="4" class="empty-state">Scanning... ${statusData.progress}%</td></tr>`;
                     } else {
                         clearInterval(pollInterval);
                         renderScanResults(statusData.data);
                     }
                 } catch(e) {
                     clearInterval(pollInterval);
                     tbody.innerHTML = '<tr><td colspan="4" class="empty-state" style="color:red">Poll Error: ' + e.message + '</td></tr>';
                 }
             }, 1000);
             
         } catch(e) {
             tbody.innerHTML = '<tr><td colspan="4" class="empty-state" style="color:red">Start Error: ' + e.message + '</td></tr>';
         }
      }
      
      async function performAction(action) {
         if(!confirm("Are you sure you want to perform: " + action + "?")) return;
         try {
             // For hardware reset actions, specific toast
             if(action.startsWith("rst")) showToast("Sending reset command...");
             
             const res = await fetch("/api/action", {
                 method: "POST",
                 body: new URLSearchParams({action: action})
             });
             const txt = await res.text();
             if(res.ok) showToast(txt);
             else showToast("Error: " + txt, "error");
         } catch(e) { showToast("Network Error", "error"); }
      }

      function renderScanResults(data) {
         const tbody = document.querySelector("#table-scanner tbody");
         if(!data || data.length === 0) {
             tbody.innerHTML = '<tr><td colspan="4" class="empty-state">No devices found.</td></tr>';
             return;
         }

         let html = "";
         data.forEach(d => {
             html += `<tr>
                <td><div style="font-weight:bold">${d.hostname}</div></td>
                <td><a href="http://${d.ip}:${d.port}" target="_blank">${d.ip}</a></td>
                <td>
                    <div style="font-size:12px; font-weight:bold; color:#555;">${d.mac || "-"}</div>
                    <div style="font-size:11px; color:#888;">${d.vendor || ""}</div>
                </td>
                <td>${d.port}</td>
                <td style="display:flex; gap:5px;">
                    <button class="btn btn-sm btn-primary" onclick="document.getElementById('ws-host').value='${d.ip}'; document.getElementById('ws-port').value='${d.port}'; initWebSocket(true);">
                        Link
                    </button>
                    <a href="http://${d.ip}:${d.port}" target="_blank" class="btn btn-sm" style="background:#28a745; text-decoration:none; display:inline-block; line-height:1.2;">
                        Open
                    </a>
                </td>
             </tr>`;
         });
         tbody.innerHTML = html;
      }

      async function loadWsServerStatus() {
         const elCount = document.getElementById("ws-server-count");
         if(!elCount) return;
         try {
             const res = await fetch("/api/ws/status");
             const data = await res.json();
             if(data.count !== undefined) elCount.innerText = data.count;
         } catch(e) { console.error("WS Stat error", e); }
      }

      function toggleProfileDropdown() {
          const dd = document.getElementById("profile-dropdown");
          if(dd) {
              dd.classList.toggle("show");
          }
      }

      // Start WS
      window.addEventListener('load', () => {
          initWebSocket(false);
          
          const params = new URLSearchParams(window.location.search);
          const savedPage = localStorage.getItem("activePage");
          
          if (params.has("uid")) {
              switchPage("add");
              document.getElementById("uid").value = params.get("uid");
          } else if (savedPage) {
              switchPage(savedPage);
          } else {
              switchPage("dashboard");
          }
           checkLoginStatus();
           setInterval(checkLoginStatus, 10000); // Polling every 10 seconds
       });
)=====";
