// g00j小站 Vue3 前端应用

const { createApp } = Vue;

createApp({
    data() {
        return {
            // 当前视图
            currentView: 'home',
            
            // 用户信息
            user: null,
            
            // 文件相关
            files: [],
            recentFiles: [],
            loading: false,
            selectedCategory: '',
            pagination: {
                page: 1,
                per_page: 20,
                total: 0,
                total_pages: 0
            },
            
            // 系统监控
            systemStatus: {},
            processes: [],
            processFilter: '',
            processSortBy: 'cpu',
            sortOrder: 'desc',
            
            // 模态框状态
            showLoginModal: false,
            showUploadModal: false,
            showPreviewModal: false,
            isRegister: false,
            
            // 表单数据
            loginForm: {
                username: '',
                password: ''
            },
            
            // 文件上传
            uploadFiles: [],
            uploading: false,
            
            // 文件预览
            previewFile: {},
            previewContent: '',
            
            // 消息提示
            message: '',
            messageType: 'success',
            
            // 注册表单
            registerForm: {
                username: '',
                password: ''
            },
            
            // 统计数据
            totalFiles: 0,
            onlineUsers: 1,
            
            // 管理面板数据
            adminUsers: [],
            adminFiles: [],
            
            // 预览相关
            showPreviewModal: false,
            previewFileData: null,
            previewContent: ''
        };
    },
    
    async created() {
        // 配置axios以自动处理cookies
        axios.defaults.withCredentials = true;
        
        // 检查登录状态
        await this.checkLoginStatus();
        
        // 加载最新文件
        await this.loadFiles();
        
        // 如果是管理员，加载系统状态
        if (this.user && this.user.role === 'admin') {
            await this.loadSystemStatus();
        }
        
        // 定期刷新系统状态
        setInterval(() => {
            if (this.user && this.user.role === 'admin') {
                this.loadSystemStatus();
            }
        }, 5000);
    },
    
    methods: {
        // 用户认证
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
                    // 设置用户信息
                    this.user = {
                        username: this.loginForm.username,
                        role: this.loginForm.username === 'admin' ? 'admin' : 'user'
                    };
                    
                    this.showMessage('登录成功！', 'success');
                    this.showLoginModal = false;
                    this.isRegister = false;
                    
                    // 清空登录表单
                    const savedUsername = this.loginForm.username;
                    this.loginForm = { username: '', password: '' };
                    
                    // 如果是管理员，加载系统状态
                    if (this.user.role === 'admin') {
                        await this.loadSystemStatus();
                    }
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
                    this.showMessage(response.data.error || '注册失败', 'error');
                }
            } catch (error) {
                this.showMessage('网络错误，请重试', 'error');
                console.error('Register error:', error);
            }
        },
        
        async logout() {
            try {
                await axios.post('/api/logout');
                this.user = null;
                this.showMessage('已登出', 'info');
                this.currentView = 'home';
            } catch (error) {
                this.showMessage('登出失败', 'error');
            }
        },
        
        async checkLoginStatus() {
            // 检查是否有session cookie
            const cookies = document.cookie.split(';');
            const sessionCookie = cookies.find(cookie => cookie.trim().startsWith('session_id='));
            
            if (sessionCookie && sessionCookie.trim() !== 'session_id=') {
                try {
                    // 通过API验证session是否有效
                    const response = await axios.get('/api/user/profile');
                    if (response.data.success) {
                        // session有效，恢复用户状态
                        this.user = response.data.data;
                        console.log('Session restored for user:', this.user.username);
                        
                        // 如果是管理员，加载系统状态
                        if (this.user.role === 'admin') {
                            await this.loadSystemStatus();
                        }
                    } else {
                        // session无效，清除cookie
                        document.cookie = 'session_id=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
                        this.user = null;
                    }
                } catch (error) {
                    // API调用失败，清除session
                    console.log('Session validation failed:', error.message);
                    document.cookie = 'session_id=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
                    this.user = null;
                }
            } else {
                this.user = null;
            }
        },
        
        // 文件管理
        async loadFiles() {
            this.loading = true;
            try {
                const params = {
                    page: this.pagination.page,
                    category: this.selectedCategory
                };
                
                const response = await axios.get('/api/files', { params });
                
                if (response.data.success) {
                    this.files = response.data.data;
                    this.pagination = response.data.pagination;
                    this.totalFiles = this.pagination.total;
                    
                    // 如果是首页，保存最新文件
                    if (this.currentView === 'home') {
                        this.recentFiles = this.files.slice(0, 6);
                    }
                }
            } catch (error) {
                this.showMessage('加载文件失败', 'error');
            } finally {
                this.loading = false;
            }
        },
        
        changePage(page) {
            if (page >= 1 && page <= this.pagination.total_pages) {
                this.pagination.page = page;
                this.loadFiles();
            }
        },
        
        // 文件上传
        handleFileSelect(event) {
            const files = Array.from(event.target.files);
            this.uploadFiles.push(...files);
        },
        
        handleDrop(event) {
            event.preventDefault();
            const files = Array.from(event.dataTransfer.files);
            this.uploadFiles.push(...files);
        },
        
        removeUploadFile(index) {
            this.uploadFiles.splice(index, 1);
        },
        
        async uploadSelectedFiles() {
            if (this.uploadFiles.length === 0) return;
            
            this.uploading = true;
            
            try {
                for (const file of this.uploadFiles) {
                    const formData = new FormData();
                    formData.append('file', file);
                    formData.append('category', 'documents');  // 默认分类
                    
                    const response = await axios.post('/api/upload', formData, {
                        headers: {
                            'Content-Type': 'multipart/form-data'
                        }
                    });
                    
                    if (response.data.success) {
                        this.showMessage(`${file.name} 上传成功！`, 'success');
                    } else {
                        this.showMessage(`${file.name} 上传失败: ${response.data.message}`, 'error');
                    }
                }
                
                // 刷新文件列表
                await this.loadFiles();
                
            } catch (error) {
                this.showMessage('上传失败，请重试', 'error');
                console.error('Upload error:', error);
            } finally {
                this.uploading = false;
                this.uploadFiles = [];
                this.showUploadModal = false;
            }
        },
        
        // 文件预览
        async previewFile(file) {
            this.previewFileData = file;
            this.showPreviewModal = true;
            
            if (this.isTextFile(file.filename)) {
                try {
                    const response = await axios.get(`/api/download?id=${file.id}`, {
                        responseType: 'text'
                    });
                    this.previewContent = response.data;
                } catch (error) {
                    this.previewContent = '无法加载文件内容';
                }
            } else {
                this.previewContent = '此文件类型不支持预览';
            }
        },
        
        downloadFile(file) {
            if (!this.user) {
                this.showMessage('请先登录', 'warning');
                return;
            }
            
            const link = document.createElement('a');
            link.href = `/api/download?id=${file.id}`;
            link.download = file.filename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        },
        
        // 系统监控
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
        
        // 进程工具函数
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
        
        // 管理面板功能
        async loadAdminUsers() {
            try {
                const response = await axios.get('/api/admin/users');
                if (response.data.success) {
                    this.adminUsers = response.data.data;
                }
            } catch (error) {
                this.showMessage('加载用户列表失败', 'error');
            }
        },

        async loadAdminFiles() {
            try {
                const response = await axios.get('/api/files?page=1&limit=50');
                if (response.data.success) {
                    this.adminFiles = response.data.data;
                }
            } catch (error) {
                this.showMessage('加载文件列表失败', 'error');
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
                    await this.loadFiles(); // 刷新普通文件列表
                } else {
                    this.showMessage(response.data.message || '删除失败', 'error');
                }
            } catch (error) {
                this.showMessage('删除文件失败', 'error');
            }
        },

        // 工具函数
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
        
        // 模态框管理
        closeModal() {
            this.showLoginModal = false;
            this.showUploadModal = false;
            this.showPreviewModal = false;
            this.isRegister = false;
            this.loginForm = { username: '', password: '' };
            this.uploadFiles = [];
            this.previewFileData = {};
            this.previewContent = '';
        },
        
        // 消息提示
        showMessage(text, type = 'info') {
            this.message = { text, type };
            setTimeout(() => {
                this.message = null;
            }, 3000);
        }
    },
    
    computed: {
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
            if (newView === 'files') {
                this.loadFiles();
            } else if (newView === 'monitor' && this.user?.role === 'admin') {
                this.loadSystemStatus();
                this.loadProcesses();
            } else if (newView === 'admin' && this.user?.role === 'admin') {
                this.loadAdminUsers();
                this.loadAdminFiles();
            }
        },
        
        selectedCategory() {
            this.pagination.page = 1;
            this.loadFiles();
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