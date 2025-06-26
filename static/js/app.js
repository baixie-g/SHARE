// 全局变量
let currentUser = null;
let authToken = null;

// 工具函数
function showToast(title, message, type = 'info') {
    const toast = document.getElementById('toast');
    const toastTitle = document.getElementById('toast-title');
    const toastMessage = document.getElementById('toast-message');
    const toastHeader = toast.querySelector('.toast-header');
    
    // 设置样式
    toastHeader.className = 'toast-header';
    if (type === 'success') {
        toastHeader.classList.add('bg-success', 'text-white');
    } else if (type === 'error') {
        toastHeader.classList.add('bg-danger', 'text-white');
    } else if (type === 'warning') {
        toastHeader.classList.add('bg-warning');
    }
    
    toastTitle.textContent = title;
    toastMessage.textContent = message;
    
    const bsToast = new bootstrap.Toast(toast);
    bsToast.show();
}

function formatFileSize(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatDate(dateString) {
    const date = new Date(dateString);
    return date.toLocaleDateString('zh-CN') + ' ' + date.toLocaleTimeString('zh-CN');
}

function getFileIcon(category) {
    const icons = {
        'video': 'bi-play-circle-fill video',
        'image': 'bi-image-fill image',
        'document': 'bi-file-text-fill document',
        'archive': 'bi-file-zip-fill archive',
        'other': 'bi-file-earmark-fill other'
    };
    return icons[category] || icons['other'];
}

// API 请求函数
async function apiRequest(url, options = {}) {
    const config = {
        headers: {
            'Content-Type': 'application/json',
            ...options.headers
        },
        ...options
    };
    
    if (authToken) {
        config.headers['Authorization'] = `Bearer ${authToken}`;
    }
    
    try {
        const response = await fetch(url, config);
        const data = await response.json();
        
        if (!response.ok) {
            throw new Error(data.error || '请求失败');
        }
        
        return data;
    } catch (error) {
        console.error('API请求错误:', error);
        throw error;
    }
}

// 认证相关函数
async function login(username, password) {
    try {
        const data = await apiRequest('/api/login', {
            method: 'POST',
            body: JSON.stringify({ username, password })
        });
        
        if (data.success) {
            currentUser = data.user;
            updateUI();
            showToast('登录成功', `欢迎回来，${username}！`, 'success');
            
            // 关闭登录模态框
            const loginModal = bootstrap.Modal.getInstance(document.getElementById('loginModal'));
            loginModal.hide();
            
            return true;
        }
    } catch (error) {
        showToast('登录失败', error.message, 'error');
        return false;
    }
}

async function register(username, password) {
    try {
        const data = await apiRequest('/api/register', {
            method: 'POST',
            body: JSON.stringify({ username, password })
        });
        
        if (data.success) {
            showToast('注册成功', '请使用新账户登录', 'success');
            
            // 关闭注册模态框
            const registerModal = bootstrap.Modal.getInstance(document.getElementById('registerModal'));
            registerModal.hide();
            
            // 显示登录模态框
            const loginModal = new bootstrap.Modal(document.getElementById('loginModal'));
            loginModal.show();
            
            return true;
        }
    } catch (error) {
        showToast('注册失败', error.message, 'error');
        return false;
    }
}

function logout() {
    currentUser = null;
    authToken = null;
    updateUI();
    showToast('退出成功', '已安全退出登录', 'success');
    
    // 清除cookie
    document.cookie = 'session_id=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
    
    // 重新加载页面以清除状态
    setTimeout(() => {
        window.location.reload();
    }, 1000);
}

function checkAuthStatus() {
    // 检查cookie中是否有session_id
    const cookies = document.cookie.split(';');
    let sessionId = null;
    
    for (let cookie of cookies) {
        const [name, value] = cookie.trim().split('=');
        if (name === 'session_id') {
            sessionId = value;
            break;
        }
    }
    
    if (sessionId) {
        // 验证session有效性
        fetch('/api/user', {
            credentials: 'include'
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                currentUser = data.user;
                updateUI();
            }
        })
        .catch(() => {
            // session无效，清除cookie
            logout();
        });
    }
}

function updateUI() {
    const authNav = document.getElementById('auth-nav');
    const userNav = document.getElementById('user-nav');
    const uploadNav = document.getElementById('upload-nav');
    const adminNav = document.getElementById('admin-nav');
    const usernameDisplay = document.getElementById('username-display');
    
    if (currentUser) {
        authNav.style.display = 'none';
        userNav.style.display = 'block';
        uploadNav.style.display = 'block';
        usernameDisplay.textContent = currentUser.username;
        
        if (currentUser.role === 'admin') {
            adminNav.style.display = 'block';
        }
    } else {
        authNav.style.display = 'block';
        userNav.style.display = 'none';
        uploadNav.style.display = 'none';
        adminNav.style.display = 'none';
    }
}

// 文件相关函数
async function loadRecentFiles() {
    try {
        const data = await apiRequest('/api/files');
        const fileList = document.getElementById('file-list');
        
        if (data.files && data.files.length > 0) {
            fileList.innerHTML = data.files.slice(0, 10).map(file => `
                <tr>
                    <td>
                        <i class="bi ${getFileIcon(file.category)} file-icon"></i>
                        <span>${file.filename}</span>
                    </td>
                    <td>
                        <span class="badge bg-secondary">${file.category}</span>
                    </td>
                    <td class="file-size">${formatFileSize(file.size)}</td>
                    <td class="text-muted">${formatDate(file.upload_time)}</td>
                    <td>
                        <div class="btn-group btn-group-sm">
                            <button class="btn btn-outline-primary" onclick="previewFile('${file.path}', '${file.category}')">
                                <i class="bi bi-eye"></i> 预览
                            </button>
                            ${currentUser ? `
                                <button class="btn btn-outline-success" onclick="downloadFile('${file.path}')">
                                    <i class="bi bi-download"></i> 下载
                                </button>
                            ` : ''}
                        </div>
                    </td>
                </tr>
            `).join('');
        } else {
            fileList.innerHTML = `
                <tr>
                    <td colspan="5" class="text-center text-muted py-4">
                        <i class="bi bi-inbox fs-1 d-block mb-2"></i>
                        暂无共享文件
                    </td>
                </tr>
            `;
        }
    } catch (error) {
        console.error('加载文件列表失败:', error);
        document.getElementById('file-list').innerHTML = `
            <tr>
                <td colspan="5" class="text-center text-danger py-4">
                    <i class="bi bi-exclamation-triangle fs-1 d-block mb-2"></i>
                    加载失败
                </td>
            </tr>
        `;
    }
}

async function loadStats() {
    try {
        const data = await apiRequest('/api/stats');
        document.getElementById('file-count').textContent = data.file_count || 0;
        document.getElementById('user-count').textContent = data.user_count || 0;
    } catch (error) {
        console.error('加载统计信息失败:', error);
    }
}

function previewFile(filePath, category) {
    // 创建预览模态框
    const modalId = 'filePreviewModal';
    let modal = document.getElementById(modalId);
    
    if (!modal) {
        modal = document.createElement('div');
        modal.className = 'modal fade';
        modal.id = modalId;
        modal.innerHTML = `
            <div class="modal-dialog modal-lg">
                <div class="modal-content">
                    <div class="modal-header">
                        <h5 class="modal-title">
                            <i class="bi bi-eye me-2"></i>
                            文件预览
                        </h5>
                        <button type="button" class="btn-close" data-bs-dismiss="modal"></button>
                    </div>
                    <div class="modal-body text-center" id="preview-content">
                        <!-- 预览内容将在这里加载 -->
                    </div>
                </div>
            </div>
        `;
        document.body.appendChild(modal);
    }
    
    const previewContent = document.getElementById('preview-content');
    
    // 根据文件类型生成预览内容
    switch (category) {
        case 'video':
            previewContent.innerHTML = `
                <video class="video-preview" controls>
                    <source src="${filePath}" type="video/mp4">
                    您的浏览器不支持视频播放。
                </video>
            `;
            break;
        case 'image':
            previewContent.innerHTML = `
                <img src="${filePath}" class="file-preview" alt="图片预览">
            `;
            break;
        case 'document':
            previewContent.innerHTML = `
                <div class="text-start">
                    <div class="spinner-border" role="status">
                        <span class="visually-hidden">加载中...</span>
                    </div>
                    <span class="ms-2">正在加载文档...</span>
                </div>
            `;
            // 异步加载文档内容
            loadDocumentContent(filePath, previewContent);
            break;
        default:
            previewContent.innerHTML = `
                <div class="text-muted">
                    <i class="bi bi-file-earmark fs-1 d-block mb-3"></i>
                    <p>此文件类型不支持预览</p>
                    <a href="${filePath}" class="btn btn-primary" target="_blank">
                        <i class="bi bi-box-arrow-up-right me-1"></i>
                        在新窗口中打开
                    </a>
                </div>
            `;
    }
    
    const bsModal = new bootstrap.Modal(modal);
    bsModal.show();
}

async function loadDocumentContent(filePath, container) {
    try {
        const response = await fetch(filePath);
        const text = await response.text();
        
        container.innerHTML = `
            <div class="document-preview">${text}</div>
        `;
    } catch (error) {
        container.innerHTML = `
            <div class="text-danger">
                <i class="bi bi-exclamation-triangle fs-1 d-block mb-3"></i>
                <p>无法加载文档内容</p>
            </div>
        `;
    }
}

function downloadFile(filePath) {
    if (!currentUser) {
        showToast('下载失败', '请先登录', 'warning');
        return;
    }
    
    // 创建临时链接进行下载
    const link = document.createElement('a');
    link.href = filePath;
    link.download = '';
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

// 表单处理
document.getElementById('loginForm').addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const username = document.getElementById('loginUsername').value;
    const password = document.getElementById('loginPassword').value;
    
    if (await login(username, password)) {
        this.reset();
    }
});

document.getElementById('registerForm').addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const username = document.getElementById('registerUsername').value;
    const password = document.getElementById('registerPassword').value;
    const confirmPassword = document.getElementById('confirmPassword').value;
    
    // 验证输入
    if (username.length < 3 || username.length > 20) {
        showToast('注册失败', '用户名长度必须在 3-20 个字符之间', 'error');
        return;
    }
    
    if (password.length < 6) {
        showToast('注册失败', '密码长度至少 6 个字符', 'error');
        return;
    }
    
    if (password !== confirmPassword) {
        showToast('注册失败', '两次输入的密码不一致', 'error');
        return;
    }
    
    if (await register(username, password)) {
        this.reset();
    }
});

// 页面路由处理 - SPA路由系统
let currentPage = 'home';

function navigateTo(path) {
    // 移除开头的斜杠
    path = path.replace(/^\//, '');
    
    // 如果路径为空，设为home
    if (!path) path = 'home';
    
    // 更新当前页面
    currentPage = path;
    
    // 更新浏览器历史记录
    window.history.pushState({page: path}, '', `/${path}`);
    
    // 渲染页面
    renderPage(path);
}

function renderPage(page) {
    const container = document.querySelector('.container.mt-4');
    
    // 隐藏所有现有内容
    const existingContent = container.innerHTML;
    
    switch(page) {
        case 'files':
            renderFilesPage();
            break;
        case 'upload':
            renderUploadPage();
            break;
        case 'admin':
            renderAdminPage();
            break;
        case 'profile':
            renderProfilePage();
            break;
        case 'my-files':
            renderMyFilesPage();
            break;
        default:
            renderHomePage();
    }
}

function renderHomePage() {
    const container = document.querySelector('.container.mt-4');
    container.innerHTML = `
        <!-- 欢迎区域 -->
        <div class="row mb-4">
            <div class="col-md-8">
                <div class="card border-0 shadow-sm">
                    <div class="card-body">
                        <h2 class="card-title text-primary">
                            <i class="bi bi-house-heart me-2"></i>
                            欢迎来到 g00j小站
                        </h2>
                        <p class="card-text text-muted">
                            轻量级文件共享系统，支持视频播放、文档阅读、图片预览等功能。
                            游客可以预览文件，注册用户可以上传和下载文件。
                        </p>
                        <div class="d-flex gap-2">
                            <button class="btn btn-primary" onclick="navigateTo('files')">
                                <i class="bi bi-folder2-open me-1"></i>
                                浏览文件
                            </button>
                            <button class="btn btn-outline-primary" data-bs-toggle="modal" data-bs-target="#registerModal">
                                <i class="bi bi-person-plus me-1"></i>
                                立即注册
                            </button>
                        </div>
                    </div>
                </div>
            </div>
            <div class="col-md-4">
                <div class="card border-0 shadow-sm">
                    <div class="card-body">
                        <h5 class="card-title">
                            <i class="bi bi-graph-up text-success me-2"></i>
                            系统统计
                        </h5>
                        <div class="row text-center">
                            <div class="col-6">
                                <div class="text-primary fs-4 fw-bold" id="file-count">-</div>
                                <small class="text-muted">共享文件</small>
                            </div>
                            <div class="col-6">
                                <div class="text-success fs-4 fw-bold" id="user-count">-</div>
                                <small class="text-muted">注册用户</small>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <!-- 最新文件 -->
        <div class="row">
            <div class="col-12">
                <div class="card border-0 shadow-sm">
                    <div class="card-header bg-white border-0 d-flex justify-content-between align-items-center">
                        <h5 class="mb-0">
                            <i class="bi bi-clock-history text-primary me-2"></i>
                            最新共享文件
                        </h5>
                        <button class="btn btn-outline-primary btn-sm" onclick="navigateTo('files')">
                            查看全部 <i class="bi bi-arrow-right"></i>
                        </button>
                    </div>
                    <div class="card-body p-0">
                        <div id="recent-files" class="table-responsive">
                            <table class="table table-hover mb-0">
                                <thead class="table-light">
                                    <tr>
                                        <th><i class="bi bi-file-earmark me-1"></i>文件名</th>
                                        <th><i class="bi bi-tag me-1"></i>类型</th>
                                        <th><i class="bi bi-hdd me-1"></i>大小</th>
                                        <th><i class="bi bi-calendar me-1"></i>上传时间</th>
                                        <th><i class="bi bi-gear me-1"></i>操作</th>
                                    </tr>
                                </thead>
                                <tbody id="file-list">
                                    <tr>
                                        <td colspan="5" class="text-center text-muted py-4">
                                            <div class="spinner-border spinner-border-sm me-2" role="status"></div>
                                            加载中...
                                        </td>
                                    </tr>
                                </tbody>
                            </table>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    // 重新加载数据
    loadRecentFiles();
    loadStats();
}

function renderFilesPage() {
    const container = document.querySelector('.container.mt-4');
    container.innerHTML = `
        <div class="row">
            <div class="col-12">
                <div class="card border-0 shadow-sm">
                    <div class="card-header bg-white border-0">
                        <h4 class="mb-0">
                            <i class="bi bi-folder2-open text-primary me-2"></i>
                            文件浏览
                        </h4>
                    </div>
                    <div class="card-body">
                        <div class="row mb-3">
                            <div class="col-md-6">
                                <input type="text" class="form-control" placeholder="搜索文件..." id="file-search">
                            </div>
                            <div class="col-md-6">
                                <select class="form-select" id="category-filter">
                                    <option value="">所有类型</option>
                                    <option value="video">视频</option>
                                    <option value="image">图片</option>
                                    <option value="document">文档</option>
                                    <option value="archive">压缩包</option>
                                    <option value="other">其他</option>
                                </select>
                            </div>
                        </div>
                        <div class="table-responsive">
                            <table class="table table-hover">
                                <thead class="table-light">
                                    <tr>
                                        <th>文件名</th>
                                        <th>类型</th>
                                        <th>大小</th>
                                        <th>上传时间</th>
                                        <th>操作</th>
                                    </tr>
                                </thead>
                                <tbody id="all-files-list">
                                    <tr>
                                        <td colspan="5" class="text-center py-4">
                                            <div class="spinner-border" role="status"></div>
                                            <div class="mt-2">加载中...</div>
                                        </td>
                                    </tr>
                                </tbody>
                            </table>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    loadAllFiles();
}

function renderUploadPage() {
    if (!currentUser) {
        showToast('访问提示', '请先登录才能上传文件', 'warning');
        navigateTo('home');
        return;
    }
    
    const container = document.querySelector('.container.mt-4');
    container.innerHTML = `
        <div class="row">
            <div class="col-12">
                <div class="card border-0 shadow-sm">
                    <div class="card-header bg-white border-0">
                        <h4 class="mb-0">
                            <i class="bi bi-cloud-upload text-primary me-2"></i>
                            文件上传
                        </h4>
                    </div>
                    <div class="card-body">
                        <div class="upload-area" id="upload-area">
                            <div class="text-center">
                                <i class="bi bi-cloud-upload fs-1 text-primary mb-3"></i>
                                <h5>拖拽文件到此处上传</h5>
                                <p class="text-muted">或者点击选择文件</p>
                                <input type="file" id="file-input" multiple style="display: none;">
                                <button class="btn btn-primary" onclick="document.getElementById('file-input').click()">
                                    <i class="bi bi-plus-circle me-1"></i>
                                    选择文件
                                </button>
                            </div>
                        </div>
                        
                        <div id="upload-progress" style="display: none;" class="mt-4">
                            <h6>上传进度</h6>
                            <div class="progress">
                                <div class="progress-bar" role="progressbar" style="width: 0%"></div>
                            </div>
                            <small class="text-muted mt-1 d-block" id="upload-status">准备上传...</small>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    setupUploadHandlers();
}

function renderAdminPage() {
    if (!currentUser || currentUser.role !== 'admin') {
        showToast('访问拒绝', '您没有管理员权限', 'error');
        navigateTo('home');
        return;
    }
    
    const container = document.querySelector('.container.mt-4');
    container.innerHTML = `
        <div class="row">
            <div class="col-12">
                <div class="card border-0 shadow-sm">
                    <div class="card-header bg-white border-0">
                        <h4 class="mb-0">
                            <i class="bi bi-gear text-primary me-2"></i>
                            系统管理
                        </h4>
                    </div>
                    <div class="card-body">
                        <div class="row">
                            <div class="col-md-3">
                                <div class="list-group">
                                    <button class="list-group-item list-group-item-action active" data-tab="system-monitor">
                                        <i class="bi bi-cpu me-2"></i>
                                        系统监控
                                    </button>
                                    <button class="list-group-item list-group-item-action" data-tab="user-management">
                                        <i class="bi bi-people me-2"></i>
                                        用户管理
                                    </button>
                                    <button class="list-group-item list-group-item-action" data-tab="file-management">
                                        <i class="bi bi-files me-2"></i>
                                        文件管理
                                    </button>
                                </div>
                            </div>
                            <div class="col-md-9">
                                <div id="admin-content">
                                    <div class="text-center py-5">
                                        <div class="spinner-border" role="status"></div>
                                        <div class="mt-2">加载管理面板...</div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    loadSystemMonitor();
}

function renderProfilePage() {
    if (!currentUser) {
        showToast('访问提示', '请先登录', 'warning');
        navigateTo('home');
        return;
    }
    
    const container = document.querySelector('.container.mt-4');
    container.innerHTML = `
        <div class="row">
            <div class="col-12">
                <div class="card border-0 shadow-sm">
                    <div class="card-header bg-white border-0">
                        <h4 class="mb-0">
                            <i class="bi bi-person-circle text-primary me-2"></i>
                            个人资料
                        </h4>
                    </div>
                    <div class="card-body">
                        <div class="row">
                            <div class="col-md-4 text-center">
                                <i class="bi bi-person-circle fs-1 text-primary"></i>
                                <h5 class="mt-2">${currentUser.username}</h5>
                                <span class="badge bg-primary">${currentUser.role === 'admin' ? '管理员' : '用户'}</span>
                            </div>
                            <div class="col-md-8">
                                <form id="profile-form">
                                    <div class="mb-3">
                                        <label class="form-label">用户名</label>
                                        <input type="text" class="form-control" value="${currentUser.username}" readonly>
                                    </div>
                                    <div class="mb-3">
                                        <label class="form-label">用户角色</label>
                                        <input type="text" class="form-control" value="${currentUser.role === 'admin' ? '管理员' : '普通用户'}" readonly>
                                    </div>
                                    <div class="mb-3">
                                        <label class="form-label">新密码（留空不修改）</label>
                                        <input type="password" class="form-control" id="new-password">
                                    </div>
                                    <div class="mb-3">
                                        <label class="form-label">确认新密码</label>
                                        <input type="password" class="form-control" id="confirm-new-password">
                                    </div>
                                    <button type="submit" class="btn btn-primary">
                                        <i class="bi bi-check-circle me-1"></i>
                                        更新资料
                                    </button>
                                </form>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;
}

function renderMyFilesPage() {
    if (!currentUser) {
        showToast('访问提示', '请先登录', 'warning');
        navigateTo('home');
        return;
    }
    
    const container = document.querySelector('.container.mt-4');
    container.innerHTML = `
        <div class="row">
            <div class="col-12">
                <div class="card border-0 shadow-sm">
                    <div class="card-header bg-white border-0">
                        <h4 class="mb-0">
                            <i class="bi bi-folder-check text-primary me-2"></i>
                            我的文件
                        </h4>
                    </div>
                    <div class="card-body">
                        <div class="table-responsive">
                            <table class="table table-hover">
                                <thead class="table-light">
                                    <tr>
                                        <th>文件名</th>
                                        <th>类型</th>
                                        <th>大小</th>
                                        <th>上传时间</th>
                                        <th>操作</th>
                                    </tr>
                                </thead>
                                <tbody id="my-files-list">
                                    <tr>
                                        <td colspan="5" class="text-center py-4">
                                            <div class="spinner-border" role="status"></div>
                                            <div class="mt-2">加载我的文件...</div>
                                        </td>
                                    </tr>
                                </tbody>
                            </table>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    `;
    
    loadMyFiles();
}

// 辅助函数
async function loadAllFiles() {
    // TODO: 实现加载所有文件的逻辑
    console.log('加载所有文件...');
}

function setupUploadHandlers() {
    // TODO: 实现文件上传处理逻辑
    console.log('设置上传处理器...');
}

function loadSystemMonitor() {
    // TODO: 实现系统监控数据加载
    console.log('加载系统监控...');
}

async function loadMyFiles() {
    // TODO: 实现加载用户文件逻辑
    console.log('加载我的文件...');
}

// 浏览器前进后退按钮处理
window.addEventListener('popstate', function(e) {
    if (e.state && e.state.page) {
        renderPage(e.state.page);
    } else {
        renderPage('home');
    }
});

// 键盘快捷键
document.addEventListener('keydown', function(e) {
    // Ctrl+L 快速登录
    if (e.ctrlKey && e.key === 'l') {
        e.preventDefault();
        if (!currentUser) {
            const loginModal = new bootstrap.Modal(document.getElementById('loginModal'));
            loginModal.show();
        }
    }
    
    // ESC 关闭模态框
    if (e.key === 'Escape') {
        const modals = document.querySelectorAll('.modal.show');
        modals.forEach(modal => {
            const bsModal = bootstrap.Modal.getInstance(modal);
            if (bsModal) {
                bsModal.hide();
            }
        });
    }
});

// 全局错误处理
window.addEventListener('error', function(e) {
    console.error('页面错误:', e.error);
    showToast('系统错误', '页面发生错误，请刷新重试', 'error');
});

// 在线状态检测
window.addEventListener('online', function() {
    showToast('网络连接', '网络连接已恢复', 'success');
});

window.addEventListener('offline', function() {
    showToast('网络连接', '网络连接已断开', 'warning');
});

// 页面加载完成后执行
document.addEventListener('DOMContentLoaded', function() {
    checkAuthStatus();
    
    // 检查当前路径并渲染对应页面
    const path = window.location.pathname.replace(/^\//, '') || 'home';
    renderPage(path);
    
    // 为按钮样式设置CSS
    const style = document.createElement('style');
    style.textContent = `
        .nav-link.btn {
            background: none !important;
            color: rgba(255, 255, 255, 0.9) !important;
            text-decoration: none !important;
            padding: 0.5rem 1rem !important;
        }
        .nav-link.btn:hover {
            color: white !important;
        }
        .dropdown-item.btn {
            background: none !important;
            color: inherit !important;
            text-decoration: none !important;
            padding: 0.25rem 1rem !important;
        }
    `;
    document.head.appendChild(style);
});