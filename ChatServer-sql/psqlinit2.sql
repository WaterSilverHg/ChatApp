-- =====================================================
-- 扩展模块
-- =====================================================
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- 用户表
CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT uuid_generate_v4(),
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    phone VARCHAR(20) UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    avatar_url TEXT,
    status VARCHAR(20) DEFAULT 'offline' CHECK (status IN ('online', 'offline', 'away', 'busy')),
    last_login TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP WITH TIME ZONE,
    
    CONSTRAINT users_email_check CHECK (email ~* '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$'),
    CONSTRAINT users_phone_check CHECK (phone IS NULL OR phone ~* '^\+?[1-9]\d{1,14}$')
);

CREATE INDEX idx_users_username ON users(username) WHERE deleted_at IS NULL;
CREATE INDEX idx_users_email ON users(email) WHERE deleted_at IS NULL;
-- CREATE INDEX idx_users_uuid ON users(uuid);
CREATE INDEX idx_users_status_deleted ON users(status, deleted_at);

COMMENT ON TABLE users IS 'Users table';
COMMENT ON COLUMN users.username IS 'Username, unique';
COMMENT ON COLUMN users.email IS 'Email address, unique';
COMMENT ON COLUMN users.password_hash IS 'Encrypted password';
COMMENT ON COLUMN users.status IS 'User status: online, offline, away, busy';

-- 用户设置表
CREATE TABLE user_settings (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    theme VARCHAR(20) DEFAULT 'light',
    language VARCHAR(10) DEFAULT 'zh-CN',
    notification_enabled BOOLEAN DEFAULT true,
    message_preview BOOLEAN DEFAULT true,
    friend_request_auto_approve BOOLEAN DEFAULT false,
    last_active TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(user_id)
);

-- 好友关系表
CREATE TABLE friendships (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    friend_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'blocked', 'deleted')),
    remark VARCHAR(50),
    group_name VARCHAR(50) DEFAULT 'Default Group',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT friendships_no_self CHECK (user_id != friend_id),
    UNIQUE(user_id, friend_id)
);

CREATE INDEX idx_friendships_user_id ON friendships(user_id, status);
CREATE INDEX idx_friendships_friend_id ON friendships(friend_id);
--CREATE INDEX idx_friendships_status ON friendships(status);

-- 群组表
CREATE TABLE groups (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT uuid_generate_v4(),
    name VARCHAR(100) NOT NULL,
    description TEXT,
    avatar_url TEXT,
    owner_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    max_members INTEGER DEFAULT 500,
    is_public BOOLEAN DEFAULT true,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP WITH TIME ZONE
     CONSTRAINT groups_max_members_check CHECK (max_members >= 1 AND max_members <= 500)
);

CREATE INDEX idx_groups_owner ON groups(owner_id);
-- CREATE INDEX idx_groups_uuid ON groups(uuid);
CREATE INDEX idx_groups_is_public ON groups(is_public) WHERE deleted_at IS NULL;

-- 群组成员表
CREATE TABLE group_members (
    id BIGSERIAL PRIMARY KEY,
    group_id BIGINT NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role VARCHAR(20) DEFAULT 'member' CHECK (role IN ('owner', 'admin', 'member')),
    nickname VARCHAR(50),
    joined_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    last_read_msg_id BIGINT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(group_id, user_id)
);

CREATE INDEX idx_group_members_group ON group_members(group_id);
CREATE INDEX idx_group_members_user ON group_members(user_id);

-- 消息表
CREATE TABLE messages (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT uuid_generate_v4(),
    from_user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    to_user_id BIGINT,
    to_group_id BIGINT,
    message_type VARCHAR(20) NOT NULL CHECK (message_type IN ('text', 'image', 'file', 'audio', 'video', 'system')),
    content TEXT NOT NULL,
    file_url TEXT,
    file_size BIGINT,
    file_name VARCHAR(255),
    mime_type VARCHAR(100),
    metadata JSONB,
    --status VARCHAR(20) DEFAULT 'sent' CHECK (status IN ('sending', 'sent', 'delivered', 'read', 'failed')),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT messages_target_check CHECK (
        (to_user_id IS NOT NULL AND to_group_id IS NULL) OR
        (to_user_id IS NULL AND to_group_id IS NOT NULL)
    )
);

CREATE INDEX idx_messages_from_user ON messages(from_user_id, created_at);
CREATE INDEX idx_messages_to_user ON messages(to_user_id, created_at);
CREATE INDEX idx_messages_to_group ON messages(to_group_id, created_at);
-- CREATE INDEX idx_messages_uuid ON messages(uuid);
CREATE INDEX idx_messages_created_at ON messages(created_at);

-- 好友请求表
CREATE TABLE friend_requests (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT uuid_generate_v4(), 
    from_user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    to_user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    message TEXT,
    status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'rejected', 'ignored')),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT friend_requests_no_self CHECK (from_user_id != to_user_id),
    UNIQUE(from_user_id, to_user_id)
);

-- UUID 索引
CREATE INDEX idx_friend_requests_to_user ON friend_requests(to_user_id);
CREATE INDEX idx_friend_requests_from_user ON friend_requests(from_user_id);
-- CREATE UNIQUE INDEX idx_friend_requests_uuid ON friend_requests(uuid);

-- 群组加入请求表
CREATE TABLE group_requests (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT uuid_generate_v4(),
    group_id BIGINT NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
    requester_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,  -- 申请人
    message TEXT,                                   -- 申请附言
    status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'rejected')),
    reviewer_id BIGINT REFERENCES users(id) ON DELETE SET NULL,           -- 审批人（群主/管理员）
    reviewed_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    UNIQUE(group_id, requester_id)  -- 防止重复申请
);

--CREATE INDEX idx_group_requests_group ON group_requests(group_id, status);
CREATE INDEX idx_group_requests_requester ON group_requests(requester_id);

-- 文件表
CREATE TABLE files (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT uuid_generate_v4(),
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    file_name VARCHAR(255) NOT NULL,
    file_path TEXT,
    file_size BIGINT NOT NULL,
    file_type VARCHAR(100),
    mime_type VARCHAR(100),
    md5_hash VARCHAR(32),
    upload_status VARCHAR(20) DEFAULT 'uploading' CHECK (upload_status IN ('uploading', 'completed', 'failed')),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP WITH TIME ZONE
);

CREATE INDEX idx_files_user ON files(user_id);
CREATE INDEX idx_files_md5 ON files(md5_hash);
-- CREATE INDEX idx_files_uuid ON files(uuid);

-- 会话表
CREATE TABLE conversations (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    target_user_id BIGINT,
    target_group_id BIGINT,
    last_read_message_id BIGINT REFERENCES messages(id) ON DELETE SET NULL,
    last_message_id BIGINT REFERENCES messages(id) ON DELETE SET NULL,
    unread_count INTEGER DEFAULT 0,
    last_read_time TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    is_muted BOOLEAN DEFAULT false,
    
    CONSTRAINT conversations_target_check CHECK (
        (target_user_id IS NOT NULL AND target_group_id IS NULL) OR
        (target_user_id IS NULL AND target_group_id IS NOT NULL)
    ),
    UNIQUE(user_id, target_user_id),
    UNIQUE(user_id, target_group_id)
);

CREATE INDEX idx_conversations_user ON conversations(user_id, updated_at);

-- 函数和触发器
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

DROP TRIGGER IF EXISTS update_users_updated_at ON users;
DROP TRIGGER IF EXISTS update_friendships_updated_at ON friendships;
DROP TRIGGER IF EXISTS update_groups_updated_at ON groups;
DROP TRIGGER IF EXISTS update_friend_requests_updated_at ON friend_requests;
DROP TRIGGER IF EXISTS update_conversations_updated_at ON conversations;

CREATE TRIGGER update_users_updated_at BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_friendships_updated_at BEFORE UPDATE ON friendships
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_groups_updated_at BEFORE UPDATE ON groups
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_friend_requests_updated_at BEFORE UPDATE ON friend_requests
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_conversations_updated_at BEFORE UPDATE ON conversations
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- 完成提示
SELECT 'Database setup completed successfully!' AS status;