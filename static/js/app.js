// g00j小站 Vue3 前端应用

const { createApp } = Vue;

createApp({
    data() {
        return {
            // 当前视图
            currentView: 'home',
            
            // 用户信息
            user: null,
            
            // 模态框状态
            showLogin: false,
            showRegister: false,
            showUpload: false,
            showPreview: false,
            
            // 表单数据
            loginForm: {
                username: '',
                password: ''
            },
            registerForm: {
                username: '',
                password: '',
                confirmPassword: ''
            },
            
            // 文件相关
            files: [],
            recentFiles: [],
            filterCategory: '',
            previewFile: null,
            previewContent: '',
            
            // 系统监控
            systemStatus: {
                cpu_usage: 0,
                memory_usage: 0,
                disk_usage: 0,
                uptime: '',
                process_count: 0,
                load_average: 0
            },
            processes: [],
            
            // 状态
            loading: false,
            error: null
        }
    },
    
    computed: {
        // 过滤后的文件列表
        filteredFiles() {
            if (!this.filterCategory) {
                return this.files;
            }
            return this.files.filter(file => file.category === this.filterCategory);
        }
    },
    
    mounted() {
        // 初始化应用
        this.init();
    },
    
    methods: {
        // 初始化
        async init() {
            await this.checkLoginStatus();
            await this.loadFiles();
            await this.loadRecentFiles();
            
            // 如果是管理员，加载系统状态
            if (this.user && this.user.role === 'admin') {
                await this.loadSystemStatus();
                // 定期更新系统状态
                setInterval(() => {
                    if (this.currentView === 'monitor') {
                        this.loadSystemStatus();
                        this.loadProcesses();
                    }
                }, 5000);
            }
        },
        
        // 检查登录状态
        async checkLoginStatus() {
            try {
                const response = await axios.get('/api/user/status');
                if (response.data.success) {
                    this.user = response.data.data;
                }
            } catch (error) {
                // 未登录，忽略错误
            }
        },
        
        // 用户登录
        async login() {
            try {
                this.loading = true;
                const response = await axios.post('/api/login', this.loginForm);
                
                if (response.data.success) {
                    this.user = response.data.data;
                    this.showLogin = false;
                    this.loginForm = { username: '', password: '' };
                    this.showMessage('登录成功！', 'success');
                    
                    // 重新加载数据
                    await this.init();
                } else {
                    this.showMessage(response.data.error || '登录失败', 'error');
                }
            } catch (error) {
                this.showMessage('网络错误，请重试', 'error');
            } finally {
                this.loading = false;
            }
        },
        
        // 用户注册
        async register() {
            if (this.registerForm.password !== this.registerForm.confirmPassword) {
                this.showMessage('两次输入的密码不一致', 'error');
                return;
            }
            
            try {
                this.loading = true;
                const response = await axios.post('/api/register', {
                    username: this.registerForm.username,
                    password: this.registerForm.password
                });
                
                if (response.data.success) {
                    this.showRegister = false;
                    this.registerForm = { username: '', password: '', confirmPassword: '' };
                    this.showMessage('注册成功！请登录', 'success');
                } else {
                    this.showMessage(response.data.error || '注册失败', 'error');
                }
            } catch (error) {
                this.showMessage('网络错误，请重试', 'error');
            } finally {
                this.loading = false;
            }
        },
        
        // 用户登出
        async logout() {
            try {
                await axios.post('/api/logout');
                this.user = null;
                this.currentView = 'home';
                this.showMessage('已安全登出', 'success');
            } catch (error) {
                this.showMessage('登出失败', 'error');
            }
        },
        
        // 加载文件列表
        async loadFiles() {
            try {
                const response = await axios.get('/api/files', {
                    params: {
                        limit: 100,
                        offset: 0,
                        category: this.filterCategory
                    }
                });
                
                if (response.data.success) {
                    this.files = response.data.data;
                }
            } catch (error) {
                this.showMessage('加载文件列表失败', 'error');
            }
        },
        
        // 加载最新文件
        async loadRecentFiles() {
            try {
                const response = await axios.get('/api/files', {
                    params: {
                        limit: 6,
                        offset: 0
                    }
                });
                
                if (response.data.success) {
                    this.recentFiles = response.data.data;
                }
            } catch (error) {
                console.error('加载最新文件失败:', error);
            }
        },
        
        // 加载系统状态
        async loadSystemStatus() {
            if (!this.user || this.user.role !== 'admin') return;
            
            try {
                const response = await axios.get('/api/system/status');
                if (response.data.success) {
                    this.systemStatus = response.data.data;
                }
            } catch (error) {
                console.error('加载系统状态失败:', error);
            }
        },
        
        // 加载进程列表
        async loadProcesses() {
            if (!this.user || this.user.role !== 'admin') return;
            
            try {
                const response = await axios.get('/api/system/processes');
                if (response.data.success) {
                    this.processes = response.data.data;
                }
            } catch (error) {
                console.error('加载进程列表失败:', error);
            }
        },
        
        // 预览文件
        async previewFile(file) {
            this.previewFile = file;
            this.showPreview = true;
            
            // 如果是文本文件，加载内容
            if (file.file_type.startsWith('text/')) {
                try {
                    const response = await axios.get(`/api/preview/${file.id}`);
                    this.previewContent = response.data;
                } catch (error) {
                    this.previewContent = '无法加载文件内容';
                }
            }
        },
        
        // 下载文件
        async downloadFile(file) {
            if (!this.user) {
                this.showMessage('请先登录后下载文件', 'error');
                return;
            }
            
            try {
                window.open(`/api/download/${file.id}`, '_blank');
            } catch (error) {
                this.showMessage('下载失败', 'error');
            }
        },
        
        // 获取文件图标
        getFileIcon(fileType) {
            if (fileType.startsWith('video/')) return '🎥';
            if (fileType.startsWith('image/')) return '🖼️';
            if (fileType.startsWith('audio/')) return '🎵';
            if (fileType.includes('pdf')) return '📄';
            if (fileType.includes('word')) return '📝';
            if (fileType.includes('excel')) return '📊';
            if (fileType.includes('zip') || fileType.includes('rar')) return '📦';
            if (fileType.startsWith('text/')) return '📄';
            return '📁';
        },
        
        // 格式化文件大小
        formatFileSize(bytes) {
            if (bytes === 0) return '0 B';
            
            const k = 1024;
            const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        },
        
        // 显示消息
        showMessage(message, type = 'info') {
            // 创建临时消息元素
            const messageEl = document.createElement('div');
            messageEl.className = `message message-${type}`;
            messageEl.textContent = message;
            messageEl.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                padding: 10px 20px;
                border-radius: 4px;
                color: white;
                font-weight: bold;
                z-index: 10000;
                animation: slideIn 0.3s ease;
            `;
            
            // 设置背景色
            switch (type) {
                case 'success':
                    messageEl.style.backgroundColor = '#4CAF50';
                    break;
                case 'error':
                    messageEl.style.backgroundColor = '#f44336';
                    break;
                case 'warning':
                    messageEl.style.backgroundColor = '#ff9800';
                    break;
                default:
                    messageEl.style.backgroundColor = '#2196F3';
            }
            
            document.body.appendChild(messageEl);
            
            // 3秒后自动移除
            setTimeout(() => {
                if (messageEl.parentNode) {
                    messageEl.parentNode.removeChild(messageEl);
                }
            }, 3000);
        },
        
        // 切换视图后的处理
        async onViewChange(view) {
            this.currentView = view;
            
            if (view === 'files') {
                await this.loadFiles();
            } else if (view === 'monitor' && this.user && this.user.role === 'admin') {
                await this.loadSystemStatus();
                await this.loadProcesses();
            }
        }
    },
    
    watch: {
        // 监听分类筛选变化
        filterCategory() {
            this.loadFiles();
        },
        
        // 监听视图变化
        currentView(newView) {
            this.onViewChange(newView);
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