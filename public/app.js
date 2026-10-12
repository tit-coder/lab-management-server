// =============================================================================
// Lab Management Server - Frontend Client Logic
// =============================================================================

const API_BASE = '/api';

// State variables
let currentDevices = [];

// DOM Elements
const healthBadge = document.getElementById('health-badge');
const uptimeText = document.getElementById('uptime-text');

const statTotal = document.getElementById('stat-total');
const statActive = document.getElementById('stat-active');
const statBroken = document.getElementById('stat-broken');
const statRepairing = document.getElementById('stat-repairing');
const statPending = document.getElementById('stat-pending');

const searchInput = document.getElementById('search-input');
const filterRoom = document.getElementById('filter-room');
const filterStatus = document.getElementById('filter-status');
const filterMaintStatus = document.getElementById('filter-maintenance-status');

const deviceCount = document.getElementById('device-count');
const deviceTableBody = document.getElementById('device-table-body');
const maintenanceTableBody = document.getElementById('maintenance-table-body');

const btnRefresh = document.getElementById('btn-refresh');
const btnAddDevice = document.getElementById('btn-add-device');

// Device Modal Elements
const deviceModal = document.getElementById('device-modal');
const modalTitle = document.getElementById('modal-title');
const deviceForm = document.getElementById('device-form');
const formDeviceId = document.getElementById('form-device-id');
const formDeviceCode = document.getElementById('form-device-code');
const formDeviceName = document.getElementById('form-device-name');
const formDeviceType = document.getElementById('form-device-type');
const formDeviceRoom = document.getElementById('form-device-room');
const formDeviceStatus = document.getElementById('form-device-status');
const modalCloseBtn = document.getElementById('modal-close-btn');
const modalCancelBtn = document.getElementById('modal-cancel-btn');

// Maintenance Modal Elements
const maintenanceModal = document.getElementById('maintenance-modal');
const maintenanceForm = document.getElementById('maintenance-form');
const formMaintDeviceId = document.getElementById('form-maint-device-id');
const maintDeviceInfo = document.getElementById('maint-device-info');
const formMaintDesc = document.getElementById('form-maint-desc');
const maintModalCloseBtn = document.getElementById('maint-modal-close-btn');
const maintModalCancelBtn = document.getElementById('maint-modal-cancel-btn');

const toastContainer = document.getElementById('toast-container');

// -----------------------------------------------------------------------------
// Toast Notification Utility
// -----------------------------------------------------------------------------
function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast ${type === 'error' ? 'toast-error' : type === 'success' ? 'toast-success' : ''}`;
    toast.textContent = message;
    toastContainer.appendChild(toast);

    setTimeout(() => {
        toast.remove();
    }, 3500);
}

// -----------------------------------------------------------------------------
// 1. Health Check
// -----------------------------------------------------------------------------
async function fetchHealth() {
    try {
        const res = await fetch(`${API_BASE}/health`);
        const data = await res.json();
        if (data.status === 'healthy') {
            healthBadge.className = 'badge badge-healthy';
            healthBadge.innerHTML = '<span class="status-dot"></span> Server & MariaDB Online';
        } else {
            healthBadge.className = 'badge badge-degraded';
            healthBadge.innerHTML = '<span class="status-dot"></span> MariaDB Chưa kết nối';
        }
        if (data.uptime_seconds !== undefined) {
            uptimeText.textContent = `Uptime: ${data.uptime_seconds}s | Port: 8080`;
        }
    } catch (err) {
        healthBadge.className = 'badge badge-degraded';
        healthBadge.innerHTML = '<span class="status-dot"></span> Không kết nối được Server';
    }
}

// -----------------------------------------------------------------------------
// 2. Fetch System Statistics
// -----------------------------------------------------------------------------
async function fetchStats() {
    try {
        const res = await fetch(`${API_BASE}/stats`);
        const json = await res.json();
        if (json.success && json.data) {
            const s = json.data;
            statTotal.textContent = s.total_devices || 0;
            statActive.textContent = s.active_devices || 0;
            statBroken.textContent = s.broken_devices || 0;
            statRepairing.textContent = s.repairing_devices || 0;
            statPending.textContent = s.pending_maintenance || 0;
        }
    } catch (err) {
        console.error('Lỗi tải thống kê:', err);
    }
}

// -----------------------------------------------------------------------------
// 3. Devices CRUD
// -----------------------------------------------------------------------------
async function fetchDevices() {
    try {
        const params = new URLSearchParams();
        if (searchInput.value.trim()) params.append('search', searchInput.value.trim());
        if (filterRoom.value) params.append('room', filterRoom.value);
        if (filterStatus.value) params.append('status', filterStatus.value);

        const res = await fetch(`${API_BASE}/devices?${params.toString()}`);
        const json = await res.json();

        if (json.success) {
            currentDevices = json.data || [];
            renderDevicesTable(currentDevices);
        } else {
            deviceTableBody.innerHTML = `<tr><td colspan="7" class="loading-cell text-danger">Lỗi: ${json.error}</td></tr>`;
        }
    } catch (err) {
        deviceTableBody.innerHTML = `<tr><td colspan="7" class="loading-cell">Không thể kết nối đến máy chủ API.</td></tr>`;
    }
}

function renderDevicesTable(devices) {
    deviceCount.textContent = devices.length;
    if (devices.length === 0) {
        deviceTableBody.innerHTML = `<tr><td colspan="7" class="loading-cell">Không có thiết bị nào phù hợp tiêu chí tìm kiếm.</td></tr>`;
        return;
    }

    deviceTableBody.innerHTML = devices.map(d => {
        let statusBadgeClass = 'badge-active';
        let statusLabel = 'Hoạt động';
        if (d.status === 'Broken') {
            statusBadgeClass = 'badge-broken';
            statusLabel = 'Hỏng hóc';
        } else if (d.status === 'Repairing') {
            statusBadgeClass = 'badge-repairing';
            statusLabel = 'Đang sửa chữa';
        }

        return `
            <tr>
                <td><strong>${escapeHtml(d.device_code)}</strong></td>
                <td>${escapeHtml(d.device_name)}</td>
                <td><span class="badge">${escapeHtml(d.device_type)}</span></td>
                <td><strong>${escapeHtml(d.room)}</strong></td>
                <td><span class="badge ${statusBadgeClass}">${statusLabel}</span></td>
                <td style="font-size:0.8rem; color:var(--text-muted);">${escapeHtml(d.created_at || '-')}</td>
                <td class="actions-cell">
                    <button class="btn btn-secondary btn-sm" onclick="handleEditDevice(${d.id})">✏️ Sửa</button>
                    <button class="btn btn-secondary btn-sm" onclick="handleReportIssue(${d.id}, '${escapeHtml(d.device_code)}', '${escapeHtml(d.device_name)}')">⚠️ Báo hỏng</button>
                    <button class="btn btn-danger btn-sm" onclick="handleDeleteDevice(${d.id}, '${escapeHtml(d.device_code)}')">🗑️ Xóa</button>
                </td>
            </tr>
        `;
    }).join('');
}

// -----------------------------------------------------------------------------
// 4. Maintenance Logs
// -----------------------------------------------------------------------------
async function fetchMaintenanceLogs() {
    try {
        const params = new URLSearchParams();
        if (filterMaintStatus.value) params.append('status', filterMaintStatus.value);

        const res = await fetch(`${API_BASE}/maintenance?${params.toString()}`);
        const json = await res.json();

        if (json.success) {
            renderMaintenanceTable(json.data || []);
        } else {
            maintenanceTableBody.innerHTML = `<tr><td colspan="7" class="loading-cell">Lỗi: ${json.error}</td></tr>`;
        }
    } catch (err) {
        maintenanceTableBody.innerHTML = `<tr><td colspan="7" class="loading-cell">Không thể kết nối đến máy chủ API.</td></tr>`;
    }
}

function renderMaintenanceTable(logs) {
    if (logs.length === 0) {
        maintenanceTableBody.innerHTML = `<tr><td colspan="7" class="loading-cell">Không có ghi nhận bảo trì nào.</td></tr>`;
        return;
    }

    maintenanceTableBody.innerHTML = logs.map(m => {
        let badgeClass = 'badge-pending';
        let label = 'Chờ xử lý';
        if (m.status === 'InProgress') {
            badgeClass = 'badge-inprogress';
            label = 'Đang xử lý';
        } else if (m.status === 'Completed') {
            badgeClass = 'badge-completed';
            label = 'Đã hoàn thành';
        }

        let actionsHtml = '';
        if (m.status === 'Pending') {
            actionsHtml = `<button class="btn btn-secondary btn-sm" onclick="changeMaintenanceStatus(${m.id}, 'InProgress')">Bắt đầu xử lý 🔧</button>`;
        } else if (m.status === 'InProgress') {
            actionsHtml = `<button class="btn btn-primary btn-sm" onclick="changeMaintenanceStatus(${m.id}, 'Completed')">Hoàn thành ✅</button>`;
        } else {
            actionsHtml = `<span style="color:var(--success); font-weight:600; font-size:0.8rem;">Đã giải quyết</span>`;
        }

        return `
            <tr>
                <td>#${m.id}</td>
                <td><strong>${escapeHtml(m.device_code || ('ID:' + m.device_id))}</strong></td>
                <td>${escapeHtml(m.issue_description)}</td>
                <td><span class="badge ${badgeClass}">${label}</span></td>
                <td style="font-size:0.8rem; color:var(--text-muted);">${escapeHtml(m.created_at || '-')}</td>
                <td style="font-size:0.8rem; color:var(--text-muted);">${escapeHtml(m.completed_at || '-')}</td>
                <td class="text-right">${actionsHtml}</td>
            </tr>
        `;
    }).join('');
}

async function changeMaintenanceStatus(id, newStatus) {
    try {
        const res = await fetch(`${API_BASE}/maintenance/${id}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ status: newStatus })
        });
        const json = await res.json();
        if (json.success) {
            showToast(`Đã cập nhật trạng thái sự cố sang: ${newStatus}`, 'success');
            refreshAll();
        } else {
            showToast(json.error || 'Cập nhật thất bại', 'error');
        }
    } catch (err) {
        showToast('Lỗi khi gửi yêu cầu cập nhật', 'error');
    }
}

// -----------------------------------------------------------------------------
// Modal Handlers & Actions
// -----------------------------------------------------------------------------
function openAddDeviceModal() {
    modalTitle.textContent = 'Thêm thiết bị mới';
    formDeviceId.value = '';
    formDeviceCode.value = '';
    formDeviceCode.disabled = false;
    formDeviceName.value = '';
    formDeviceType.value = 'PC';
    formDeviceRoom.value = 'LAB101';
    formDeviceStatus.value = 'Active';
    deviceModal.classList.remove('hidden');
}

window.handleEditDevice = function(id) {
    const dev = currentDevices.find(d => d.id === id);
    if (!dev) return;

    modalTitle.textContent = `Chỉnh sửa thiết bị #${dev.id}`;
    formDeviceId.value = dev.id;
    formDeviceCode.value = dev.device_code;
    formDeviceCode.disabled = true; // Mã thiết bị không được sửa
    formDeviceName.value = dev.device_name;
    formDeviceType.value = dev.device_type;
    formDeviceRoom.value = dev.room;
    formDeviceStatus.value = dev.status;
    deviceModal.classList.remove('hidden');
};

function closeDeviceModal() {
    deviceModal.classList.add('hidden');
}

window.handleDeleteDevice = async function(id, code) {
    if (!confirm(`Bạn có chắc chắn muốn xóa thiết bị [${code}] không?\nLưu ý: Các bản ghi bảo trì liên quan cũng sẽ bị xóa.`)) {
        return;
    }

    try {
        const res = await fetch(`${API_BASE}/devices/${id}`, { method: 'DELETE' });
        const json = await res.json();
        if (json.success) {
            showToast(`Đã xóa thiết bị ${code} thành công!`, 'success');
            refreshAll();
        } else {
            showToast(json.error || 'Không thể xóa thiết bị', 'error');
        }
    } catch (err) {
        showToast('Lỗi kết nối khi xóa thiết bị', 'error');
    }
};

window.handleReportIssue = function(deviceId, deviceCode, deviceName) {
    formMaintDeviceId.value = deviceId;
    maintDeviceInfo.textContent = `[${deviceCode}] - ${deviceName}`;
    formMaintDesc.value = '';
    maintenanceModal.classList.remove('hidden');
};

function closeMaintenanceModal() {
    maintenanceModal.classList.add('hidden');
}

// Form Submit: Device Form (Create or Update)
deviceForm.addEventListener('submit', async (e) => {
    e.preventDefault();

    const id = formDeviceId.value;
    const isEdit = Boolean(id);

    const payload = {
        device_code: formDeviceCode.value.trim(),
        device_name: formDeviceName.value.trim(),
        device_type: formDeviceType.value,
        room: formDeviceRoom.value.trim(),
        status: formDeviceStatus.value
    };

    try {
        const url = isEdit ? `${API_BASE}/devices/${id}` : `${API_BASE}/devices`;
        const method = isEdit ? 'PUT' : 'POST';

        const res = await fetch(url, {
            method: method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        const json = await res.json();
        if (json.success) {
            showToast(isEdit ? 'Cập nhật thiết bị thành công!' : 'Thêm thiết bị mới thành công!', 'success');
            closeDeviceModal();
            refreshAll();
        } else {
            showToast(json.error || 'Thao tác không thành công', 'error');
        }
    } catch (err) {
        showToast('Lỗi gửi dữ liệu lên server', 'error');
    }
});

// Form Submit: Maintenance Form
maintenanceForm.addEventListener('submit', async (e) => {
    e.preventDefault();

    const payload = {
        device_id: parseInt(formMaintDeviceId.value, 10),
        issue_description: formMaintDesc.value.trim(),
        status: 'Pending'
    };

    try {
        const res = await fetch(`${API_BASE}/maintenance`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        const json = await res.json();
        if (json.success) {
            showToast('Đã tạo phiếu bảo trì sự cố thành công!', 'success');
            closeMaintenanceModal();
            refreshAll();
        } else {
            showToast(json.error || 'Tạo phiếu bảo trì thất bại', 'error');
        }
    } catch (err) {
        showToast('Lỗi gửi dữ liệu bảo trì', 'error');
    }
});

// -----------------------------------------------------------------------------
// Utilities & Refresh All
// -----------------------------------------------------------------------------
function escapeHtml(str) {
    if (!str) return '';
    return String(str)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#039;');
}

function refreshAll() {
    fetchHealth();
    fetchStats();
    fetchDevices();
    fetchMaintenanceLogs();
}

// Event Listeners
btnAddDevice.addEventListener('click', openAddDeviceModal);
modalCloseBtn.addEventListener('click', closeDeviceModal);
modalCancelBtn.addEventListener('click', closeDeviceModal);

maintModalCloseBtn.addEventListener('click', closeMaintenanceModal);
maintModalCancelBtn.addEventListener('click', closeMaintenanceModal);

btnRefresh.addEventListener('click', () => {
    refreshAll();
    showToast('Đã làm mới dữ liệu!', 'info');
});

let debounceTimer = null;
searchInput.addEventListener('input', () => {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(fetchDevices, 300);
});

filterRoom.addEventListener('change', fetchDevices);
filterStatus.addEventListener('change', fetchDevices);
filterMaintStatus.addEventListener('change', fetchMaintenanceLogs);

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    refreshAll();
    // Poll health check every 15 seconds
    setInterval(fetchHealth, 15000);
});
