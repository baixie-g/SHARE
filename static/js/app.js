// g00j个人网盘 Vue3 前端应用

const { createApp } = Vue;

createApp({
    data() {
        return {
            // 当前视图
            currentView: 'home', // 默认显示首页
            
            // 用户信息
            user: null,
            
            // 我的文件相关
            myFiles: [],
            fileFilter: '',
            fileCategory: '',
            
            // 分享文件相关
            sharedFiles: [],
            sharedFilter: '',
            sharedCategory: '',
            sharedSortBy: 'latest',
            sharedViewMode: 'grid',
            
            // 首页数据
            recentFiles: [],
            totalFiles: 0,
            onlineUsers: 1,
            
            // 视图模式
            viewMode: 'grid',
            
            // 存储信息
            storageUsed: 0,
            storageQuota: 1073741824, // 1GB
            
            // 文件上传
            uploadFiles: [],
            uploadCategories: [],
            uploading: false,
            
            // 系统监控 (管理员功能)
            systemStatus: {},
            processes: [],
            processFilter: '',
            processSortBy: 'cpu',
            
            // 管理面板数据
            adminUsers: [],
            adminFiles: [],
            
            // 模态框状态
            showLoginModal: false,
            showUploadModal: false,
            showPreviewModal: false,
            showRegisterForm: false,
            
            // 表单数据
            loginForm: {
                username: '',
                password: ''
            },
            registerForm: {
                username: '',
                password: ''
            },
            
            // 预览相关
            previewFileData: null,
            previewContent: '',
            
            // 消息提示
            message: null
        };
    },
    
    async created() {
        // 配置axios以自动处理cookies
        axios.defaults.withCredentials = true;
        
        // 检查登录状态
        await this.checkLoginStatus();
        
        // 加载数据
        await this.loadInitialData();
        
        // 定期刷新系统状态 (管理员)
        if (this.user && this.user.role === 'admin') {
            setInterval(() => {
                if (this.currentView === 'monitor') {
                    this.loadSystemStatus();
                }
            }, 5000);
        }
    },
    
    computed: {
        // 存储使用率百分比
        storageUsagePercent() {
            return this.storageQuota > 0 ? (this.storageUsed / this.storageQuota) * 100 : 0;
        },
        
        // 过滤后的我的文件
        filteredMyFiles() {
            let files = this.myFiles;
            
            // 按分类过滤
            if (this.fileCategory) {
                files = files.filter(file => file.category === this.fileCategory);
            }
            
            // 按关键词过滤
            if (this.fileFilter) {
                const filter = this.fileFilter.toLowerCase();
                files = files.filter(file => 
                    file.filename.toLowerCase().includes(filter)
                );
            }
            
            return files;
        },
        
        // 过滤后的分享文件
        filteredSharedFiles() {
            let files = this.sharedFiles;
            
            // 按分类过滤
            if (this.sharedCategory) {
                files = files.filter(file => file.category === this.sharedCategory);
            }
            
            // 按关键词过滤
            if (this.sharedFilter) {
                const filter = this.sharedFilter.toLowerCase();
                files = files.filter(file => 
                    file.filename.toLowerCase().includes(filter) ||
                    file.uploader.toLowerCase().includes(filter)
                );
            }
            
            return files;
        },
        
        // 过滤后的进程列表 (管理员功能)
        filteredProcesses() {
            if (!this.processFilter) {
                return this.processes;
            }
            
            const filter = this.processFilter.toLowerCase();
            return this.processes.filter(process => 
                process.name.toLowerCase().includes(filter) ||
                process.user.toLowerCase().includes(filter) ||
                process.pid.includes(filter)
            );
        }
    },
    
    watch: {
        currentView(newView) {
            console.log('🔄 视图切换到:', newView, '当前用户:', this.user);
            
            if (newView === 'home') {
                this.loadHomeData();
            } else if (newView === 'myfiles' && this.user) {
                this.loadMyFiles();
                this.loadUserStorage();
            } else if (newView === 'shared') {
                this.loadSharedFiles();
            } else if (newView === 'monitor' && this.user?.role === 'admin') {
                this.loadSystemStatus();
                this.loadProcesses();
            } else if (newView === 'admin' && this.user?.role === 'admin') {
                console.log('👑 管理员进入管理面板，开始加载数据...');
                this.loadAdminPanel();
            } else {
                console.log('⚠️ 视图切换条件不满足 - 视图:', newView, '用户角色:', this.user?.role);
            }
        }
    },
    
    methods: {
        // === 初始化和用户管理 ===
        
        async loadInitialData() {
            // 根据当前视图和用户状态加载数据
            if (this.currentView === 'home') {
                await this.loadHomeData();
            } else if (this.currentView === 'shared') {
                await this.loadSharedFiles();
            }
            
            if (this.user) {
                if (this.currentView === 'myfiles') {
                    await this.loadMyFiles();
                    await this.loadUserStorage();
                } else if (this.currentView === 'admin' && this.user.role === 'admin') {
                    await this.loadAdminPanel();
                } else if (this.currentView === 'monitor' && this.user.role === 'admin') {
                    await this.loadSystemStatus();
                    await this.loadProcesses();
                }
            }
        },
        
        async checkLoginStatus() {
            // 检查是否有session cookie
            const cookies = document.cookie.split(';');
            const sessionCookie = cookies.find(cookie => cookie.trim().startsWith('session_id='));
            
            if (sessionCookie && sessionCookie.trim() !== 'session_id=') {
                try {
                    const response = await axios.get('/api/user/profile');
                    if (response.data.success) {
                        this.user = response.data.data;
                        console.log('🔑 Session restored for user:', this.user.username, '角色:', this.user.role);
                        
                        // 如果用户已登录，但仍在首页，保持首页
                        if (this.currentView === 'shared') {
                            this.currentView = 'home';
                        }
                    } else {
                        document.cookie = 'session_id=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
                        this.user = null;
                    }
                } catch (error) {
                    console.log('Session validation failed:', error.message);
                    document.cookie = 'session_id=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
                    this.user = null;
                }
            } else {
                this.user = null;
            }
        },
        
        async login() {
            try {
                const formData = new URLSearchParams();
                formData.append('username', this.loginForm.username);
                formData.append('password', this.loginForm.password);
                
                const response = await axios.post('/api/login', formData, {
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    },
                    withCredentials: true
                });
                
                if (response.data.success) {
                    // 登录成功后，获取真实的用户信息
                    try {
                        const profileResponse = await axios.get('/api/user/profile');
                        if (profileResponse.data.success) {
                            this.user = profileResponse.data.data;
                        } else {
                            // 如果获取用户信息失败，使用基本信息
                            this.user = {
                                username: this.loginForm.username,
                                role: this.loginForm.username === 'admin' ? 'admin' : 'user'
                            };
                        }
                    } catch (error) {
                        // 如果获取用户信息失败，使用基本信息
                        this.user = {
                            username: this.loginForm.username,
                            role: this.loginForm.username === 'admin' ? 'admin' : 'user'
                        };
                    }
                    
                    this.showMessage('登录成功！', 'success');
                    this.showLoginModal = false;
                    this.showRegisterForm = false;
                    this.loginForm = { username: '', password: '' };
                    
                    // 登录后返回首页
                    this.currentView = 'home';
                    await this.loadHomeData();
                } else {
                    this.showMessage(response.data.message || '登录失败', 'error');
                }
            } catch (error) {
                this.showMessage('网络错误，请重试', 'error');
                console.error('Login error:', error);
            }
        },
        
        async register() {
            try {
                const formData = new URLSearchParams();
                formData.append('username', this.registerForm.username);
                formData.append('password', this.registerForm.password);
                
                const response = await axios.post('/api/register', formData, {
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    }
                });
                
                if (response.data.success) {
                    this.showMessage('注册成功！请登录', 'success');
                    this.showRegisterForm = false;
                    this.registerForm = { username: '', password: '' };
                } else {
                    this.showMessage(response.data.message || '注册失败', 'error');
                }
            } catch (error) {
                this.showMessage('网络错误，请重试', 'error');
                console.error('Register error:', error);
            }
        },
        
        async logout() {
            try {
                await axios.post('/api/logout');
                
                // 清除本地用户状态
                this.user = null;
                this.myFiles = [];
                this.storageUsed = 0;
                this.storageQuota = 1073741824;
                
                // 清除本地存储的session信息（如果有）
                document.cookie = "session_id=; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT";
                
                // 重置视图到首页
                this.currentView = 'home';
                
                // 重新加载首页数据
                await this.loadHomeData();
                
                this.showMessage('已成功登出', 'success');
                
            } catch (error) {
                // 即使服务器返回错误，也要清除本地状态
                this.user = null;
                this.myFiles = [];
                this.storageUsed = 0;
                this.storageQuota = 1073741824;
                this.currentView = 'home';
                
                // 强制清除Cookie
                document.cookie = "session_id=; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT";
                
                this.showMessage('已登出', 'info');
            }
        },
        
        // === 首页数据加载 ===
        
        async loadHomeData() {
            try {
                // 加载分享文件作为最新文件
                await this.loadSharedFiles();
                this.recentFiles = this.sharedFiles.slice(0, 6); // 显示最新6个
                this.totalFiles = this.sharedFiles.length;
                
                // 如果是管理员，加载系统状态
                if (this.user && this.user.role === 'admin') {
                    await this.loadSystemStatus();
                }
            } catch (error) {
                console.error('Load home data failed:', error);
            }
        },
        
        // === 文件管理 ===
        
        async loadMyFiles() {
            if (!this.user) return;
            
            try {
                const response = await axios.get('/api/my-files');
                if (response.data.success) {
                    this.myFiles = response.data.data;
                }
            } catch (error) {
                this.showMessage('加载我的文件失败', 'error');
            }
        },
        
        async loadSharedFiles() {
            try {
                const response = await axios.get('/api/shared-files');
                if (response.data.success) {
                    this.sharedFiles = response.data.data;
                }
            } catch (error) {
                this.showMessage('加载分享文件失败', 'error');
            }
        },
        
        async loadUserStorage() {
            if (!this.user) return;
            
            try {
                const response = await axios.get('/api/user/storage');
                if (response.data.success) {
                    const data = response.data.data;
                    this.storageUsed = data.used;
                    this.storageQuota = data.quota;
                }
            } catch (error) {
                this.showMessage('加载存储信息失败', 'error');
            }
        },
        
        async toggleShare(file) {
            try {
                const formData = new URLSearchParams();
                formData.append('file_id', file.id);
                
                const response = await axios.post('/api/toggle-share', formData, {
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    }
                });
                
                if (response.data.success) {
                    file.is_shared = !file.is_shared;
                    const message = file.is_shared ? '文件已分享' : '已取消分享';
                    this.showMessage(message, 'success');
                    
                    // 重新加载分享文件列表
                    if (file.is_shared) {
                        await this.loadSharedFiles();
                    }
                } else {
                    this.showMessage(response.data.message || '操作失败', 'error');
                }
            } catch (error) {
                this.showMessage('操作失败', 'error');
            }
        },
        
        async deleteMyFile(file) {
            if (!confirm(`确定要删除文件 "${file.filename}" 吗？此操作不可撤销！`)) {
                return;
            }
            
            try {
                const formData = new URLSearchParams();
                formData.append('id', file.id);
                
                const response = await axios.post('/api/admin/delete-file', formData, {
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    }
                });
                
                if (response.data.success) {
                    this.showMessage('文件删除成功', 'success');
                    await this.loadMyFiles();
                    await this.loadUserStorage();
                    await this.loadSharedFiles(); // 更新分享列表
                } else {
                    this.showMessage(response.data.message || '删除失败', 'error');
                }
            } catch (error) {
                this.showMessage('删除文件失败', 'error');
            }
        },
        
        // === 文件上传 ===
        
        handleFileSelect(event) {
            const files = Array.from(event.target.files);
            this.addFilesToUpload(files);
        },
        
        handleFileDrop(event) {
            event.preventDefault();
            const files = Array.from(event.dataTransfer.files);
            this.addFilesToUpload(files);
        },
        
        addFilesToUpload(files) {
            files.forEach((file, index) => {
                this.uploadFiles.push(file);
                // 智能分类
                this.uploadCategories.push(this.detectFileCategory(file.name));
            });
        },
        
        detectFileCategory(filename) {
            const ext = filename.split('.').pop()?.toLowerCase();
            
            if (['txt', 'md', 'pdf', 'doc', 'docx', 'xls', 'xlsx', 'ppt', 'pptx'].includes(ext)) {
                return 'documents';
            } else if (['jpg', 'jpeg', 'png', 'gif', 'bmp', 'webp', 'svg'].includes(ext)) {
                return 'images';
            } else if (['mp4', 'avi', 'mkv', 'mov', 'wmv', 'flv', 'webm'].includes(ext)) {
                return 'videos';
            } else {
                return 'others';
            }
        },
        
        removeFile(index) {
            this.uploadFiles.splice(index, 1);
            this.uploadCategories.splice(index, 1);
        },
        
        async uploadSelectedFiles() {
            if (this.uploadFiles.length === 0) {
                this.showMessage('请选择要上传的文件', 'warning');
                return;
            }
            
            this.uploading = true;
            let successCount = 0;
            
            for (let i = 0; i < this.uploadFiles.length; i++) {
                const file = this.uploadFiles[i];
                const category = this.uploadCategories[i];
                
                try {
                    const formData = new FormData();
                    formData.append('file', file);
                    formData.append('category', category);
                    
                    const response = await axios.post('/api/upload', formData, {
                        headers: {
                            'Content-Type': 'multipart/form-data'
                        }
                    });
                    
                    if (response.data.success) {
                        successCount++;
                    } else {
                        this.showMessage(`上传 ${file.name} 失败: ${response.data.message}`, 'error');
                    }
                } catch (error) {
                    this.showMessage(`上传 ${file.name} 失败`, 'error');
                }
            }
            
            this.uploading = false;
            
            if (successCount > 0) {
                this.showMessage(`成功上传 ${successCount} 个文件`, 'success');
                this.uploadFiles = [];
                this.uploadCategories = [];
                this.showUploadModal = false;
                
                // 刷新数据
                await this.loadMyFiles();
                await this.loadUserStorage();
            }
        },
        
        // === 文件预览和下载 ===
        
        previewFile(file) {
            console.log('previewFile called with:', file);
            
            if (!file) {
                console.error('No file provided to preview');
                this.showMessage('预览失败：文件信息无效', 'error');
                return;
            }
            
            // 先设置基本信息并显示模态框
            this.previewFileData = file;
            this.showPreviewModal = true;
            this.previewContent = '正在加载...';
            
            console.log('Modal opened for file:', file.filename);
            
            // 异步加载内容
            this.loadPreviewContent(file);
        },
        
        async loadPreviewContent(file) {
            try {
                if (this.isTextFile(file.filename)) {
                    console.log('Loading text file content...');
                    const response = await axios.get(`/api/download?id=${file.id}`, {
                        responseType: 'text'
                    });
                    console.log('Text content loaded:', response.data.substring(0, 50));
                    this.previewContent = response.data;
                } else {
                    console.log('Non-text file, setting default message');
                    this.previewContent = '此文件类型不支持预览';
                }
            } catch (error) {
                console.error('Error loading content:', error);
                this.previewContent = '无法加载文件内容: ' + error.message;
            }
        },
        
        downloadFile(file) {
            const link = document.createElement('a');
            link.href = `/api/download?id=${file.id}`;
            link.download = file.filename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        },
        
        // === 系统监控 (管理员功能) ===
        
        async loadSystemStatus() {
            try {
                const response = await axios.get('/api/system/status');
                if (response.data.success) {
                    this.systemStatus = response.data.data;
                }
            } catch (error) {
                this.showMessage('加载系统状态失败', 'error');
            }
        },
        
        async loadProcesses() {
            try {
                const response = await axios.get('/api/system/processes');
                if (response.data.success) {
                    this.processes = response.data.data;
                    this.sortProcesses();
                }
            } catch (error) {
                this.showMessage('加载进程列表失败', 'error');
            }
        },
        
        sortProcesses() {
            this.processes.sort((a, b) => {
                let valA, valB;
                
                switch (this.processSortBy) {
                    case 'cpu':
                        valA = parseFloat(a.cpu);
                        valB = parseFloat(b.cpu);
                        return valB - valA; // 降序
                    case 'memory':
                        valA = parseFloat(a.memory);
                        valB = parseFloat(b.memory);
                        return valB - valA; // 降序
                    case 'name':
                        valA = a.name.toLowerCase();
                        valB = b.name.toLowerCase();
                        return valA.localeCompare(valB); // 升序
                    case 'pid':
                        valA = parseInt(a.pid);
                        valB = parseInt(b.pid);
                        return valA - valB; // 升序
                    case 'user':
                        valA = a.user.toLowerCase();
                        valB = b.user.toLowerCase();
                        return valA.localeCompare(valB); // 升序
                    default:
                        return 0;
                }
            });
        },
        
        async killProcess(pid) {
            if (!confirm(`确定要终止进程 ${pid} 吗？`)) {
                return;
            }
            
            try {
                const formData = new URLSearchParams();
                formData.append('pid', pid);
                
                const response = await axios.post('/api/admin/kill-process', formData, {
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    }
                });
                
                if (response.data.success) {
                    this.showMessage('进程终止成功', 'success');
                    await this.loadProcesses();
                } else {
                    this.showMessage(response.data.message || '终止进程失败', 'error');
                }
            } catch (error) {
                this.showMessage('终止进程失败', 'error');
            }
        },
        
        // === 管理面板功能 ===
        
        async loadAdminPanel() {
            console.log('👑 开始加载管理面板...');
            // 进入管理面板前，先验证一下session是否有效
            try {
                const verifyResponse = await axios.get('/api/user/profile');
                if (verifyResponse.data.success && verifyResponse.data.data.role === 'admin') {
                    console.log('✅ Session验证成功，开始加载管理员数据...');
                    // 验证成功，加载管理员数据
                    await this.loadAdminUsers();
                    await this.loadAdminFiles();
                } else {
                    console.log('⚠️ Session验证失败，清除用户状态');
                    this.user = null;
                    this.showMessage('登录状态已失效，请重新登录', 'error');
                    this.currentView = 'home';
                }
            } catch (error) {
                console.log('⚠️ Session验证异常，清除用户状态', error);
                this.user = null;
                this.showMessage('登录验证失败，请重新登录', 'error');
                this.currentView = 'home';
            }
        },

        async loadAdminUsers() {
            console.log('👥 开始加载管理员用户列表...');
            try {
                const response = await axios.get('/api/admin/users');
                console.log('📡 用户API响应:', response.data);
                
                // 处理可能的字符串响应
                let responseData = response.data;
                if (typeof responseData === 'string') {
                    try {
                        // 清理无效的Unicode转义序列
                        let cleanedData = responseData.replace(/\\u000[^\w]/g, '?');
                        cleanedData = cleanedData.replace(/\\u[0-9a-fA-F]{0,3}[^\w]/g, '?');
                        
                        responseData = JSON.parse(cleanedData);
                        console.log('🔄 用户数据JSON解析成功:', responseData);
                    } catch (parseError) {
                        console.error('❌ 用户数据JSON解析失败:', parseError);
                        this.showMessage('用户数据格式错误', 'error');
                        return;
                    }
                }
                
                if (responseData.success) {
                    this.adminUsers = responseData.data;
                    console.log('✅ 成功加载用户列表，数量:', this.adminUsers.length);
                } else {
                    console.error('❌ 用户API返回失败:', responseData.message);
                    this.showMessage('加载用户列表失败: ' + responseData.message, 'error');
                }
            } catch (error) {
                console.error('❌ 加载用户列表异常:', error);
                this.showMessage('加载用户列表失败', 'error');
            }
        },
        
        async loadAdminFiles() {
            console.log('🔧 开始加载管理员文件列表...');
            try {
                const response = await axios.get('/api/admin/files');
                console.log('📡 API响应:', response.data);
                console.log('📡 响应类型:', typeof response.data);
                
                // 处理可能的字符串响应
                let responseData = response.data;
                if (typeof responseData === 'string') {
                    try {
                        // 清理无效的Unicode转义序列
                        let cleanedData = responseData.replace(/\\u000[^\w]/g, '?');
                        // 也处理其他可能的无效转义
                        cleanedData = cleanedData.replace(/\\u[0-9a-fA-F]{0,3}[^\w]/g, '?');
                        
                        console.log('🧹 清理后的数据长度:', cleanedData.length, '原数据长度:', responseData.length);
                        
                        responseData = JSON.parse(cleanedData);
                        console.log('🔄 JSON解析成功:', responseData);
                    } catch (parseError) {
                        console.error('❌ JSON解析失败:', parseError);
                        console.error('❌ 原始数据片段:', responseData.substring(1030, 1050));
                        this.showMessage('数据格式错误', 'error');
                        this.adminFiles = [];
                        return;
                    }
                }
                
                if (responseData && responseData.success) {
                    this.adminFiles = responseData.data;
                    console.log('✅ 成功加载文件列表，数量:', this.adminFiles.length);
                    console.log('📋 文件数据:', this.adminFiles);
                } else {
                    const errorMsg = responseData ? responseData.message : '未知错误';
                    console.error('❌ API返回失败:', errorMsg);
                    this.showMessage('加载文件列表失败: ' + errorMsg, 'error');
                    this.adminFiles = []; // 清空数组
                }
            } catch (error) {
                console.error('❌ 加载文件列表异常:', error);
                
                // 检查是否是权限问题
                if (error.response && error.response.status === 401) {
                    this.showMessage('权限验证失败，请重新登录', 'error');
                    // 重新验证用户状态
                    await this.checkLoginStatus();
                } else if (error.response && error.response.data && error.response.data.message) {
                    this.showMessage('加载文件列表失败: ' + error.response.data.message, 'error');
                } else {
                    this.showMessage('加载文件列表失败: 网络错误', 'error');
                }
                this.adminFiles = []; // 清空数组
            }
        },
        
        async deleteUser(userId) {
            if (!confirm('确定要删除该用户吗？此操作不可撤销！')) {
                return;
            }
            
            try {
                const formData = new URLSearchParams();
                formData.append('id', userId);
                
                const response = await axios.post('/api/admin/delete-user', formData, {
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    }
                });
                
                if (response.data.success) {
                    this.showMessage('用户删除成功', 'success');
                    await this.loadAdminUsers();
                } else {
                    this.showMessage(response.data.message || '删除失败', 'error');
                }
            } catch (error) {
                this.showMessage('删除用户失败', 'error');
            }
        },
        
        async deleteAdminFile(fileId) {
            if (!confirm('确定要删除该文件吗？此操作不可撤销！')) {
                return;
            }
            
            try {
                const formData = new URLSearchParams();
                formData.append('id', fileId);
                
                const response = await axios.post('/api/admin/delete-file', formData, {
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    }
                });
                
                if (response.data.success) {
                    this.showMessage('文件删除成功', 'success');
                    await this.loadAdminFiles();
                } else {
                    this.showMessage(response.data.message || '删除失败', 'error');
                }
            } catch (error) {
                this.showMessage('删除文件失败', 'error');
            }
        },
        
        // === 工具函数 ===
        
        formatFileSize(bytes) {
            if (bytes === 0) return '0 B';
            const k = 1024;
            const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
        },
        
        formatDate(dateString) {
            const date = new Date(dateString);
            return date.toLocaleString('zh-CN');
        },
        
        formatDateShort(dateString) {
            const date = new Date(dateString);
            const now = new Date();
            const diffTime = Math.abs(now - date);
            const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
            
            if (diffDays <= 1) return '今天';
            if (diffDays <= 7) return `${diffDays} 天前`;
            if (diffDays <= 30) return `${Math.floor(diffDays / 7)} 周前`;
            return date.toLocaleDateString('zh-CN', { month: '2-digit', day: '2-digit' });
        },
        
        getCategoryName(category) {
            const names = {
                'documents': '📄 文档',
                'images': '🖼️ 图片', 
                'videos': '🎬 视频',
                'others': '📦 其他'
            };
            return names[category] || '📁 文件';
        },
        
        getUniqueUploaders() {
            const uploaders = [...new Set(this.sharedFiles.map(file => file.uploader))];
            return uploaders.length;
        },
        
        getTotalSharedSize() {
            return this.sharedFiles.reduce((total, file) => total + (file.size || 0), 0);
        },
        
        getFileIcon(filename) {
            const ext = filename.split('.').pop()?.toLowerCase();
            
            if (['mp4', 'avi', 'mkv', 'mov', 'wmv', 'flv', 'webm'].includes(ext)) {
                return '🎬';
            } else if (['jpg', 'jpeg', 'png', 'gif', 'bmp', 'webp', 'svg'].includes(ext)) {
                return '🖼️';
            } else if (['txt', 'md', 'pdf', 'doc', 'docx'].includes(ext)) {
                return '📄';
            } else if (['mp3', 'wav', 'flac', 'aac'].includes(ext)) {
                return '🎵';
            } else if (['zip', 'rar', '7z', 'tar', 'gz'].includes(ext)) {
                return '📦';
            } else {
                return '📄';
            }
        },
        
        isVideoFile(filename) {
            const ext = filename.split('.').pop()?.toLowerCase();
            return ['mp4', 'avi', 'mkv', 'mov', 'wmv', 'flv', 'webm'].includes(ext);
        },
        
        isImageFile(filename) {
            const ext = filename.split('.').pop()?.toLowerCase();
            return ['jpg', 'jpeg', 'png', 'gif', 'bmp', 'webp', 'svg'].includes(ext);
        },
        
        isTextFile(filename) {
            const ext = filename.split('.').pop()?.toLowerCase();
            return ['txt', 'md', 'json', 'xml', 'html', 'css', 'js'].includes(ext);
        },
        
        getCpuClass(cpu) {
            const usage = parseFloat(cpu);
            if (usage > 80) return 'high';
            if (usage > 50) return 'medium';
            if (usage > 20) return 'low';
            return 'minimal';
        },
        
        getMemoryClass(memory) {
            const usage = parseFloat(memory);
            if (usage > 80) return 'high';
            if (usage > 50) return 'medium';
            if (usage > 20) return 'low';
            return 'minimal';
        },
        
        formatNumber(num) {
            const n = parseInt(num);
            if (n > 1024 * 1024) {
                return (n / (1024 * 1024)).toFixed(1) + 'G';
            } else if (n > 1024) {
                return (n / 1024).toFixed(1) + 'M';
            }
            return n.toString();
        },
        
        // 模态框管理
        closeModal() {
            this.showLoginModal = false;
            this.showUploadModal = false;
            this.showPreviewModal = false;
            this.showRegisterForm = false;
            this.loginForm = { username: '', password: '' };
            this.registerForm = { username: '', password: '' };
            this.uploadFiles = [];
            this.uploadCategories = [];
            this.previewFileData = null;
            this.previewContent = '';
        },
        
        // 消息提示
        showMessage(text, type = 'info') {
            this.message = { text, type };
            setTimeout(() => {
                this.message = null;
            }, 3000);
        }
    }
}).mount('#app');

// 添加动画样式
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from {
            transform: translateX(100%);
            opacity: 0;
        }
        to {
            transform: translateX(0);
            opacity: 1;
        }
    }
`;
document.head.appendChild(style); 