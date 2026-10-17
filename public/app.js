/**
 * ====================================================================
 * Frontend Controller for Computer Lab Management System
 * Pure Vanilla JavaScript with Fetch API
 * Connects directly to C++ HTTP RESTful Server & MariaDB Backend
 * ====================================================================
 */

document.addEventListener("DOMContentLoaded", () => {
    // State
    let currentDevices = [];
    let deleteTargetId = null;

    // DOM Elements
    const systemHealth = document.getElementById("systemHealth");
    const healthText = document.getElementById("healthText");
    const dbAlert = document.getElementById("dbAlert");

    // Stats
    const statTotal = document.getElementById("statTotal");
    const statActive = document.getElementById("statActive");
    const statBroken = document.getElementById("statBroken");
    const statRepairing = document.getElementById("statRepairing");

    // Tabs
    const tabButtons = document.querySelectorAll(".tab-btn");
    const tabPanes = document.querySelectorAll(".tab-pane");

    // Devices View
    const deviceSearch = document.getElementById("deviceSearch");
    const deviceRoomFilter = document.getElementById("deviceRoomFilter");
    const deviceStatusFilter = document.getElementById("deviceStatusFilter");
    const btnRefreshDevices = document.getElementById("btnRefreshDevices");
    const devicesTableBody = document.getElementById("devicesTableBody");
    const btnOpenAddDeviceModal = document.getElementById("btnOpenAddDeviceModal");

    // Maintenance View
    const maintStatusFilter = document.getElementById("maintStatusFilter");
    const btnRefreshMaintenance = document.getElementById("btnRefreshMaintenance");
    const maintTableBody = document.getElementById("maintTableBody");
    const btnOpenAddMaintModal = document.getElementById("btnOpenAddMaintModal");

    // Modals
    const addDeviceModal = document.getElementById("addDeviceModal");
    const addDeviceForm = document.getElementById("addDeviceForm");
    const editDeviceModal = document.getElementById("editDeviceModal");
    const editDeviceForm = document.getElementById("editDeviceForm");
    const addMaintModal = document.getElementById("addMaintModal");
    const addMaintForm = document.getElementById("addMaintForm");
    const maintDeviceSelect = document.getElementById("maintDeviceSelect");
    const updateMaintModal = document.getElementById("updateMaintModal");
    const updateMaintForm = document.getElementById("updateMaintForm");
    const deleteConfirmModal = document.getElementById("deleteConfirmModal");
    const btnConfirmDelete = document.getElementById("btnConfirmDelete");
    const deleteDeviceCodeText = document.getElementById("deleteDeviceCodeText");

    // Toast Container
    const toastContainer = document.getElementById("toastContainer");

    // =========================================================
    // Utilities: Toast & Modal Management
    // =========================================================
    function showToast(message, type = "info") {
        const toast = document.createElement("div");
        toast.className = `toast toast-${type}`;
        
        let icon = "ℹ️";
        if (type === "success") icon = "✅";
        if (type === "error") icon = "❌";
        if (type === "warning") icon = "⚠️";

        toast.innerHTML = `<span>${icon}</span> <span>${escapeHtml(message)}</span>`;
        toastContainer.appendChild(toast);

        setTimeout(() => {
            toast.style.opacity = "0";
            toast.style.transform = "translateX(40px)";
            setTimeout(() => toast.remove(), 250);
        }, 4000);
    }

    function openModal(modal) {
        modal.classList.add("open");
    }

    function closeModal(modal) {
        modal.classList.remove("open");
    }

    document.querySelectorAll("[data-close-modal]").forEach(btn => {
        btn.addEventListener("click", () => {
            const modal = btn.closest(".modal-backdrop");
            if (modal) closeModal(modal);
        });
    });

    window.addEventListener("click", (e) => {
        if (e.target.classList.contains("modal-backdrop")) {
            closeModal(e.target);
        }
    });

    function escapeHtml(str) {
        if (str === null || str === undefined) return "";
        return String(str)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#039;");
    }

    function getBadgeHtml(status, isMaintenance = false) {
        if (!isMaintenance) {
            if (status === "Active") return `<span class="badge badge-active">● Hoạt động</span>`;
            if (status === "Broken") return `<span class="badge badge-broken">● Gặp sự cố</span>`;
            if (status === "Repairing") return `<span class="badge badge-repairing">● Đang sửa</span>`;
            return `<span class="badge">${escapeHtml(status)}</span>`;
        } else {
            if (status === "Pending") return `<span class="badge badge-pending">🕒 Chờ xử lý</span>`;
            if (status === "InProgress") return `<span class="badge badge-inprogress">🔧 Đang sửa</span>`;
            if (status === "Completed") return `<span class="badge badge-completed">✅ Đã xong</span>`;
            return `<span class="badge">${escapeHtml(status)}</span>`;
        }
    }

    // =========================================================
    // Tab Switching
    // =========================================================
    tabButtons.forEach(btn => {
        btn.addEventListener("click", () => {
            const targetId = btn.dataset.tab;
            tabButtons.forEach(b => b.classList.remove("active"));
            tabPanes.forEach(p => p.classList.remove("active"));

            btn.classList.add("active");
            const targetPane = document.getElementById(targetId);
            if (targetPane) targetPane.classList.add("active");

            if (targetId === "tabDevices") {
                loadDevices();
                loadStats();
            } else if (targetId === "tabMaintenance") {
                loadMaintenance();
            }
        });
    });

    // =========================================================
    // API Calls: Health Check
    // =========================================================
    async function checkHealth() {
        try {
            const res = await fetch("/api/health");
            const data = await res.json();

            if (res.ok && data.database === true) {
                systemHealth.className = "status-pill status-online";
                healthText.textContent = "Hệ thống & MariaDB: Trực tuyến";
                dbAlert.classList.add("hidden");
            } else {
                systemHealth.className = "status-pill status-offline";
                healthText.textContent = "Cơ sở dữ liệu MariaDB mất kết nối";
                dbAlert.classList.remove("hidden");
            }
        } catch (err) {
            systemHealth.className = "status-pill status-offline";
            healthText.textContent = "Không thể kết nối C++ Web Server";
            dbAlert.classList.remove("hidden");
        }
    }

    // =========================================================
    // API Calls: Stats
    // =========================================================
    async function loadStats() {
        try {
            const res = await fetch("/api/stats");
            const json = await res.json();
            if (res.ok && json.success) {
                const d = json.data;
               statTotal.textContent = d.total_devices ?? 0;
               statActive.textContent = d.active_devices ?? 0;
               statBroken.textContent = d.broken_devices ?? 0;
               statRepairing.textContent = d.repairing_devices ?? 0;
            }
        } catch (err) {
            console.error("Lỗi tải thống kê:", err);
        }
    }

    // =========================================================
    // API Calls: Devices View
    // =========================================================
    async function loadDevices() {
        devicesTableBody.innerHTML = `
            <tr>
                <td colspan="8" class="text-center py-4 text-muted">
                    Đang tải dữ liệu từ C++ Backend...
                </td>
            </tr>
        `;

        const room = deviceRoomFilter.value;
        const status = deviceStatusFilter.value;
        const search = deviceSearch.value.trim();

        const params = new URLSearchParams();
        if (room) params.append("room", room);
        if (status) params.append("status", status);
        if (search) params.append("search", search);

        try {
            const res = await fetch(`/api/devices?${params.toString()}`);
            const json = await res.json();

            if (!res.ok || !json.success) {
                devicesTableBody.innerHTML = `
                    <tr>
                        <td colspan="8" class="text-center py-4 text-danger">
                            ❌ ${escapeHtml(json.error || "Không thể tải danh sách thiết bị")}
                        </td>
                    </tr>
                `;
                return;
            }

            currentDevices = json.data;
            renderDevicesTable(currentDevices);
            updateDeviceSelectOptions(currentDevices);
        } catch (err) {
            devicesTableBody.innerHTML = `
                <tr>
                    <td colspan="8" class="text-center py-4 text-danger">
                        ❌ Lỗi kết nối mạng tới C++ Server
                    </td>
                </tr>
            `;
        }
    }

    function renderDevicesTable(devices) {
        if (!devices || devices.length === 0) {
            devicesTableBody.innerHTML = `
                <tr>
                    <td colspan="8" class="text-center py-4 text-muted">
                        Không có thiết bị nào phù hợp với bộ lọc.
                    </td>
                </tr>
            `;
            return;
        }

        devicesTableBody.innerHTML = devices.map(d => `
            <tr>
                <td><strong>#${d.id}</strong></td>
                <td><code>${escapeHtml(d.device_code)}</code></td>
                <td><strong>${escapeHtml(d.device_name)}</strong></td>
                <td>${escapeHtml(d.device_type)}</td>
                <td>📍 ${escapeHtml(d.room)}</td>
                <td>${getBadgeHtml(d.status)}</td>
                <td class="text-muted" style="font-size: 0.8rem;">${escapeHtml(d.created_at || "--")}</td>
                <td class="table-actions">
                    <button class="btn btn-secondary btn-sm" onclick="window.onEditDevice(${d.id})" title="Sửa thông tin">
                        ✏️ Sửa
                    </button>
                    <button class="btn btn-warning btn-sm" onclick="window.onReportIssue(${d.id})" title="Báo sự cố">
                        ⚠️ Báo hỏng
                    </button>
                    <button class="btn btn-danger btn-sm" onclick="window.onDeleteDevice(${d.id}, '${escapeHtml(d.device_code)}')" title="Xóa thiết bị">
                        🗑️ Xóa
                    </button>
                </td>
            </tr>
        `).join("");
    }

    function updateDeviceSelectOptions(devices) {
        maintDeviceSelect.innerHTML = `<option value="">-- Chọn thiết bị --</option>` +
            devices.map(d => `<option value="${d.id}">#${d.id} - ${escapeHtml(d.device_code)} (${escapeHtml(d.device_name)} - ${escapeHtml(d.room)})</option>`).join("");
    }

    // Filter events
    deviceRoomFilter.addEventListener("change", loadDevices);
    deviceStatusFilter.addEventListener("change", loadDevices);
    btnRefreshDevices.addEventListener("click", () => {
        loadDevices();
        loadStats();
        checkHealth();
    });

    let searchTimer = null;
    deviceSearch.addEventListener("input", () => {
        clearTimeout(searchTimer);
        searchTimer = setTimeout(loadDevices, 300);
    });

    // =========================================================
    // Add Device Modal & Action
    // =========================================================
    btnOpenAddDeviceModal.addEventListener("click", () => {
        addDeviceForm.reset();
        openModal(addDeviceModal);
    });

    addDeviceForm.addEventListener("submit", async (e) => {
        e.preventDefault();
        const payload = {
            device_code: document.getElementById("newDeviceCode").value.trim(),
            device_name: document.getElementById("newDeviceName").value.trim(),
            device_type: document.getElementById("newDeviceType").value.trim(),
            room: document.getElementById("newDeviceRoom").value.trim(),
            status: document.getElementById("newDeviceStatus").value
        };

        const btnSubmit = document.getElementById("btnSubmitAddDevice");
        btnSubmit.disabled = true;
        btnSubmit.textContent = "Đang lưu...";

        try {
            const res = await fetch("/api/devices", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(payload)
            });
            const json = await res.json();

            if (res.status === 201 && json.success) {
                showToast("Thêm thiết bị mới thành công!", "success");
                closeModal(addDeviceModal);
                loadDevices();
                loadStats();
            } else {
                showToast(json.error || "Thêm thiết bị thất bại", "error");
            }
        } catch (err) {
            showToast("Lỗi kết nối máy chủ", "error");
        } finally {
            btnSubmit.disabled = false;
            btnSubmit.textContent = "Lưu thiết bị";
        }
    });

    // =========================================================
    // Edit Device Action
    // =========================================================
    window.onEditDevice = async function(id) {
        try {
            const res = await fetch(`/api/devices/${id}`);
            const json = await res.json();
            if (res.ok && json.success) {
                const d = json.data;
                document.getElementById("editDeviceId").value = d.id;
                document.getElementById("editDeviceCode").value = d.device_code;
                document.getElementById("editDeviceName").value = d.device_name;
                document.getElementById("editDeviceType").value = d.device_type;
                document.getElementById("editDeviceRoom").value = d.room;
                document.getElementById("editDeviceStatus").value = d.status;

                openModal(editDeviceModal);
            } else {
                showToast(json.error || "Không thể lấy thông tin thiết bị", "error");
            }
        } catch (err) {
            showToast("Lỗi kết nối máy chủ", "error");
        }
    };

    editDeviceForm.addEventListener("submit", async (e) => {
        e.preventDefault();
        const id = document.getElementById("editDeviceId").value;
        const payload = {
            device_code: document.getElementById("editDeviceCode").value.trim(),
            device_name: document.getElementById("editDeviceName").value.trim(),
            device_type: document.getElementById("editDeviceType").value.trim(),
            room: document.getElementById("editDeviceRoom").value.trim(),
            status: document.getElementById("editDeviceStatus").value
        };

        const btnSubmit = document.getElementById("btnSubmitEditDevice");
        btnSubmit.disabled = true;
        btnSubmit.textContent = "Đang cập nhật...";

        try {
            const res = await fetch(`/api/devices/${id}`, {
                method: "PUT",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(payload)
            });
            const json = await res.json();

            if (res.ok && json.success) {
                showToast("Cập nhật thông tin thiết bị thành công!", "success");
                closeModal(editDeviceModal);
                loadDevices();
                loadStats();
            } else {
                showToast(json.error || "Cập nhật thất bại", "error");
            }
        } catch (err) {
            showToast("Lỗi kết nối máy chủ", "error");
        } finally {
            btnSubmit.disabled = false;
            btnSubmit.textContent = "Cập nhật";
        }
    });

    // =========================================================
    // Delete Device Action (With History Preservation Notice)
    // =========================================================
    window.onDeleteDevice = function(id, code) {
        deleteTargetId = id;
        deleteDeviceCodeText.textContent = code;
        openModal(deleteConfirmModal);
    };

    btnConfirmDelete.addEventListener("click", async () => {
        if (!deleteTargetId) return;

        btnConfirmDelete.disabled = true;
        btnConfirmDelete.textContent = "Đang xóa...";

        try {
            const res = await fetch(`/api/devices/${deleteTargetId}`, {
                method: "DELETE"
            });
            const json = await res.json();

            if (res.ok && json.success) {
                showToast("Đã xóa thiết bị thành công", "success");
                closeModal(deleteConfirmModal);
                loadDevices();
                loadStats();
            } else if (res.status === 409) {
                // Conflict due to preservation policy!
                showToast(json.error || "Không thể xóa: Đã có lịch sử bảo trì liên kết!", "warning");
                closeModal(deleteConfirmModal);
            } else {
                showToast(json.error || "Không thể xóa thiết bị", "error");
                closeModal(deleteConfirmModal);
            }
        } catch (err) {
            showToast("Lỗi kết nối máy chủ khi xóa", "error");
        } finally {
            btnConfirmDelete.disabled = false;
            btnConfirmDelete.textContent = "Đồng ý xóa";
            deleteTargetId = null;
        }
    });

    // =========================================================
    // Maintenance View & Actions
    // =========================================================
    btnOpenAddMaintModal.addEventListener("click", () => {
        addMaintForm.reset();
        openModal(addMaintModal);
    });

    window.onReportIssue = function(deviceId) {
        addMaintForm.reset();
        maintDeviceSelect.value = deviceId;
        openModal(addMaintModal);
    };

    addMaintForm.addEventListener("submit", async (e) => {
        e.preventDefault();
        const deviceId = parseInt(maintDeviceSelect.value, 10);
        const description = document.getElementById("maintDescription").value.trim();
        const status = document.getElementById("maintStatus").value;

        if (!deviceId) {
            showToast("Vui lòng chọn thiết bị", "warning");
            return;
        }

        const btnSubmit = document.getElementById("btnSubmitAddMaint");
        btnSubmit.disabled = true;
        btnSubmit.textContent = "Đang ghi nhận...";

        try {
            const res = await fetch("/api/maintenance", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    device_id: deviceId,
                    description: description,
                    status: status
                })
            });
            const json = await res.json();

            if (res.status === 201 && json.success) {
                showToast("Đã ghi nhận sự cố và tạo phiếu bảo trì!", "success");
                closeModal(addMaintModal);
                loadMaintenance();
                loadDevices();
                loadStats();
            } else {
                showToast(json.error || "Không thể tạo phiếu bảo trì", "error");
            }
        } catch (err) {
            showToast("Lỗi kết nối máy chủ", "error");
        } finally {
            btnSubmit.disabled = false;
            btnSubmit.textContent = "Tạo phiếu bảo trì";
        }
    });

    async function loadMaintenance() {
        maintTableBody.innerHTML = `
            <tr>
                <td colspan="8" class="text-center py-4 text-muted">
                    Đang tải danh sách bảo trì từ C++ Backend...
                </td>
            </tr>
        `;

        const status = maintStatusFilter.value;
        const params = new URLSearchParams();
        if (status) params.append("status", status);

        try {
            const res = await fetch(`/api/maintenance?${params.toString()}`);
            const json = await res.json();

            if (!res.ok || !json.success) {
                maintTableBody.innerHTML = `
                    <tr>
                        <td colspan="8" class="text-center py-4 text-danger">
                            ❌ ${escapeHtml(json.error || "Không thể tải danh sách phiếu bảo trì")}
                        </td>
                    </tr>
                `;
                return;
            }

            renderMaintenanceTable(json.data);
        } catch (err) {
            maintTableBody.innerHTML = `
                <tr>
                    <td colspan="8" class="text-center py-4 text-danger">
                        ❌ Lỗi kết nối mạng tới C++ Server
                    </td>
                </tr>
            `;
        }
    }

    function renderMaintenanceTable(logs) {
        if (!logs || logs.length === 0) {
            maintTableBody.innerHTML = `
                <tr>
                    <td colspan="8" class="text-center py-4 text-muted">
                        Không có phiếu bảo trì nào.
                    </td>
                </tr>
            `;
            return;
        }

        maintTableBody.innerHTML = logs.map(m => `
            <tr>
                <td><strong>#${m.id}</strong></td>
                <td><code>${escapeHtml(m.device_code)}</code></td>
                <td>${escapeHtml(m.device_name)} <br><small class="text-muted">📍 ${escapeHtml(m.room)}</small></td>
                <td>${escapeHtml(m.description)}</td>
                <td>${getBadgeHtml(m.status, true)}</td>
                <td class="text-muted" style="font-size: 0.8rem;">${escapeHtml(m.created_at || "--")}</td>
                <td class="text-muted" style="font-size: 0.8rem;">${escapeHtml(m.completed_at || "--")}</td>
                <td class="text-center">
                    <button class="btn btn-secondary btn-sm" onclick="window.onOpenUpdateMaint(${m.id}, '${escapeHtml(m.device_code)} - ${escapeHtml(m.device_name)}', '${escapeHtml(m.status)}', '${escapeHtml(m.description)}')" title="Cập nhật tiến độ">
                        🔄 Cập nhật tiến độ
                    </button>
                </td>
            </tr>
        `).join("");
    }

    maintStatusFilter.addEventListener("change", loadMaintenance);
    btnRefreshMaintenance.addEventListener("click", () => {
        loadMaintenance();
        loadStats();
    });

    window.onOpenUpdateMaint = function(id, deviceText, currentStatus, currentDesc) {
        document.getElementById("updateMaintId").value = id;
        document.getElementById("updateMaintDeviceText").textContent = deviceText;
        document.getElementById("updateMaintStatus").value = currentStatus;
        document.getElementById("updateMaintDescription").value = currentDesc;
        openModal(updateMaintModal);
    };

    updateMaintForm.addEventListener("submit", async (e) => {
        e.preventDefault();
        const id = document.getElementById("updateMaintId").value;
        const status = document.getElementById("updateMaintStatus").value;
        const description = document.getElementById("updateMaintDescription").value.trim();

        const btnSubmit = document.getElementById("btnSubmitUpdateMaint");
        btnSubmit.disabled = true;
        btnSubmit.textContent = "Đang lưu...";

        try {
            const res = await fetch(`/api/maintenance/${id}`, {
                method: "PUT",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ status, description })
            });
            const json = await res.json();

            if (res.ok && json.success) {
                showToast("Cập nhật tiến độ bảo trì thành công!", "success");
                closeModal(updateMaintModal);
                loadMaintenance();
                loadDevices();
                loadStats();
            } else {
                showToast(json.error || "Không thể cập nhật phiếu bảo trì", "error");
            }
        } catch (err) {
            showToast("Lỗi kết nối máy chủ", "error");
        } finally {
            btnSubmit.disabled = false;
            btnSubmit.textContent = "Lưu tiến độ";
        }
    });

    // =========================================================
    // Initial Load & Polling
    // =========================================================
    checkHealth();
    loadStats();
    loadDevices();

    // Heartbeat every 15 seconds
    setInterval(checkHealth, 15000);
});
