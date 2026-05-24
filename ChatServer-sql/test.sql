-- =====================================================
-- 聊天室系统 - 全面测试用例（最终版）
-- 执行方式：\c chatroom 后执行 \i test_cases.sql
-- 注意：本脚本会清空所有数据，谨慎使用！
-- =====================================================

-- 清理所有表数据（级联删除，重置序列）
DO $$
BEGIN
    RAISE NOTICE '==========================================';
    RAISE NOTICE '开始清理现有数据...';
    RAISE NOTICE '==========================================';
END $$;

TRUNCATE TABLE conversations CASCADE;
TRUNCATE TABLE messages CASCADE;
TRUNCATE TABLE group_members CASCADE;
TRUNCATE TABLE groups CASCADE;
TRUNCATE TABLE friendships CASCADE;
TRUNCATE TABLE friend_requests CASCADE;
TRUNCATE TABLE files CASCADE;
TRUNCATE TABLE user_settings CASCADE;
TRUNCATE TABLE users CASCADE;

-- 重启序列
ALTER SEQUENCE users_id_seq RESTART WITH 1;
ALTER SEQUENCE groups_id_seq RESTART WITH 1;
ALTER SEQUENCE messages_id_seq RESTART WITH 1;
ALTER SEQUENCE conversations_id_seq RESTART WITH 1;
ALTER SEQUENCE friendships_id_seq RESTART WITH 1;
ALTER SEQUENCE friend_requests_id_seq RESTART WITH 1;
ALTER SEQUENCE files_id_seq RESTART WITH 1;
ALTER SEQUENCE user_settings_id_seq RESTART WITH 1;

DO $$
BEGIN
    RAISE NOTICE '数据清理完成';
    RAISE NOTICE '==========================================';
    RAISE NOTICE '开始执行测试用例';
    RAISE NOTICE '==========================================';
END $$;

-- =====================================================
-- 第一部分：用户相关测试
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 1. 用户模块测试 ===';
END $$;

-- 插入多个测试用户
INSERT INTO users (username, email, phone, password_hash, avatar_url, status) VALUES
    ('zhangwei', 'zhangwei@example.com', '+8613800000001', crypt('pass123', gen_salt('bf')), '/avatars/zhangwei.jpg', 'online'),
    ('lifang', 'lifang@example.com', '+8613800000002', crypt('pass456', gen_salt('bf')), '/avatars/lifang.jpg', 'offline'),
    ('wangqiang', 'wangqiang@example.com', '+8613800000003', crypt('pass789', gen_salt('bf')), '/avatars/wangqiang.jpg', 'away'),
    ('zhaomin', 'zhaomin@example.com', '+8613800000004', crypt('passabc', gen_salt('bf')), '/avatars/zhaomin.jpg', 'busy'),
    ('sunhao', 'sunhao@example.com', '+8613800000005', crypt('passdef', gen_salt('bf')), NULL, 'online'),
    ('chenjing', 'chenjing@example.com', '+8613800000006', crypt('passghi', gen_salt('bf')), '/avatars/chenjing.jpg', 'offline');

-- 验证用户创建数量
DO $$
DECLARE
    user_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO user_count FROM users;
    RAISE NOTICE '✓ 创建了 % 个用户', user_count;
    ASSERT user_count >= 6, '用户创建失败';
END $$;

-- 1.2 测试用户名唯一性约束（应该失败）
DO $$
BEGIN
    BEGIN
        INSERT INTO users (username, email, phone, password_hash) 
        VALUES ('zhangwei', 'duplicate@example.com', '+8613800000099', crypt('test', gen_salt('bf')));
        RAISE EXCEPTION '✗ 用户名唯一性约束测试失败：应该报错但没有';
    EXCEPTION 
        WHEN unique_violation THEN
            RAISE NOTICE '✓ 用户名唯一性约束测试通过';
    END;
END $$;

-- 1.3 测试邮箱唯一性约束（应该失败）
DO $$
BEGIN
    BEGIN
        INSERT INTO users (username, email, phone, password_hash) 
        VALUES ('testuser', 'zhangwei@example.com', '+8613800000088', crypt('test', gen_salt('bf')));
        RAISE EXCEPTION '✗ 邮箱唯一性约束测试失败：应该报错但没有';
    EXCEPTION 
        WHEN unique_violation THEN
            RAISE NOTICE '✓ 邮箱唯一性约束测试通过';
    END;
END $$;

-- 1.4 测试邮箱格式验证（应该失败）
DO $$
BEGIN
    BEGIN
        INSERT INTO users (username, email, phone, password_hash) 
        VALUES ('invaliduser', 'invalid-email', '+8613800000077', crypt('test', gen_salt('bf')));
        RAISE EXCEPTION '✗ 邮箱格式验证测试失败：应该报错但没有';
    EXCEPTION 
        WHEN check_violation THEN
            RAISE NOTICE '✓ 邮箱格式验证测试通过';
    END;
END $$;

-- 1.5 测试用户状态更新
UPDATE users SET status = 'online', last_login = CURRENT_TIMESTAMP 
WHERE username = 'lifang' AND status = 'offline';

-- 1.6 测试软删除
UPDATE users SET deleted_at = CURRENT_TIMESTAMP WHERE username = 'sunhao';

-- =====================================================
-- 第二部分：用户设置测试
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 2. 用户设置模块测试 ===';
END $$;

-- 插入用户设置
INSERT INTO user_settings (user_id, theme, language, notification_enabled, message_preview, friend_request_auto_approve, last_active)
SELECT id, 'dark', 'en-US', true, true, false, CURRENT_TIMESTAMP
FROM users WHERE username = 'zhangwei'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO user_settings (user_id, theme, language, notification_enabled, message_preview, friend_request_auto_approve)
SELECT id, 'light', 'zh-CN', false, true, true
FROM users WHERE username = 'lifang'
ON CONFLICT (user_id) DO NOTHING;

-- 验证设置
DO $$
DECLARE
    setting_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO setting_count FROM user_settings;
    RAISE NOTICE '✓ 创建了 % 条用户设置', setting_count;
END $$;

-- 测试设置更新
UPDATE user_settings 
SET theme = 'system', updated_at = CURRENT_TIMESTAMP 
WHERE user_id = (SELECT id FROM users WHERE username = 'zhangwei');

-- =====================================================
-- 第三部分：好友关系测试（保留原逻辑：friend_requests + friendships 直接插入 pending）
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 3. 好友模块测试 ===';
END $$;

-- 3.1 通过 friend_requests 创建并接受好友关系（zhangwei <-> lifang）
INSERT INTO friend_requests (from_user_id, to_user_id, message, status)
SELECT u1.id, u2.id, 'Hi, let''s be friends!', 'accepted'
FROM users u1, users u2
WHERE u1.username = 'zhangwei' AND u2.username = 'lifang'
ON CONFLICT (from_user_id, to_user_id) DO NOTHING;

-- 根据 accepted 请求创建 friendships 双向记录
INSERT INTO friendships (user_id, friend_id, status, remark, group_name)
SELECT from_user_id, to_user_id, 'accepted', 'Good friend', 'Close Friends'
FROM friend_requests WHERE status = 'accepted'
ON CONFLICT (user_id, friend_id) DO NOTHING;

INSERT INTO friendships (user_id, friend_id, status, group_name)
SELECT to_user_id, from_user_id, 'accepted', 'Close Friends'
FROM friend_requests WHERE status = 'accepted'
ON CONFLICT (user_id, friend_id) DO NOTHING;

-- 3.2 创建 friend_requests accepted 请求（lifang -> zhaomin）
INSERT INTO friend_requests (from_user_id, to_user_id, message, status)
SELECT u1.id, u2.id, 'Friend request', 'accepted'
FROM users u1, users u2
WHERE u1.username = 'lifang' AND u2.username = 'zhaomin'
ON CONFLICT (from_user_id, to_user_id) DO NOTHING;

-- 创建对应的 friendships 双向记录
INSERT INTO friendships (user_id, friend_id, status, remark, group_name)
SELECT from_user_id, to_user_id, 'accepted', 'Colleague', 'Work'
FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'lifang')
  AND to_user_id = (SELECT id FROM users WHERE username = 'zhaomin')
ON CONFLICT DO NOTHING;

INSERT INTO friendships (user_id, friend_id, status, group_name)
SELECT to_user_id, from_user_id, 'accepted', 'Work'
FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'lifang')
  AND to_user_id = (SELECT id FROM users WHERE username = 'zhaomin')
ON CONFLICT DO NOTHING;

-- 3.3 创建 pending 好友请求（chenjing -> zhangwei）
-- 注意：pending 状态只存在于 friend_requests，friendships 应该在请求被接受后才创建
INSERT INTO friend_requests (from_user_id, to_user_id, message, status)
SELECT u1.id, u2.id, 'Hi, add me please', 'pending'
FROM users u1, users u2
WHERE u1.username = 'chenjing' AND u2.username = 'zhangwei'
ON CONFLICT (from_user_id, to_user_id) DO NOTHING;

-- 3.4 创建 wangqiang <-> zhaomin 好友关系（accepted）
INSERT INTO friend_requests (from_user_id, to_user_id, message, status)
SELECT u1.id, u2.id, 'Classmate', 'accepted'
FROM users u1, users u2
WHERE u1.username = 'wangqiang' AND u2.username = 'zhaomin'
ON CONFLICT (from_user_id, to_user_id) DO NOTHING;

INSERT INTO friendships (user_id, friend_id, status, remark, group_name)
SELECT from_user_id, to_user_id, 'accepted', 'Classmate', 'School'
FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'wangqiang')
  AND to_user_id = (SELECT id FROM users WHERE username = 'zhaomin')
ON CONFLICT DO NOTHING;

INSERT INTO friendships (user_id, friend_id, status, group_name)
SELECT to_user_id, from_user_id, 'accepted', 'School'
FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'wangqiang')
  AND to_user_id = (SELECT id FROM users WHERE username = 'zhaomin')
ON CONFLICT DO NOTHING;

-- 3.5 创建 zhangwei <-> wangqiang 好友关系（accepted）- 修复：他们之间有消息往来应该是好友
INSERT INTO friend_requests (from_user_id, to_user_id, message, status)
SELECT u1.id, u2.id, 'Nice to meet you!', 'accepted'
FROM users u1, users u2
WHERE u1.username = 'zhangwei' AND u2.username = 'wangqiang'
ON CONFLICT (from_user_id, to_user_id) DO NOTHING;

INSERT INTO friendships (user_id, friend_id, status, remark, group_name)
SELECT from_user_id, to_user_id, 'accepted', 'Tech friend', 'Technology'
FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'zhangwei')
  AND to_user_id = (SELECT id FROM users WHERE username = 'wangqiang')
ON CONFLICT DO NOTHING;

INSERT INTO friendships (user_id, friend_id, status, group_name)
SELECT to_user_id, from_user_id, 'accepted', 'Technology'
FROM friend_requests WHERE from_user_id = (SELECT id FROM users WHERE username = 'zhangwei')
  AND to_user_id = (SELECT id FROM users WHERE username = 'wangqiang')
ON CONFLICT DO NOTHING;

-- 验证好友关系（统计 accepted + pending）
DO $$
DECLARE
    accepted_count INTEGER;
    pending_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO accepted_count FROM friendships WHERE status = 'accepted';
    SELECT COUNT(*) INTO pending_count FROM friendships WHERE status = 'pending';
    RAISE NOTICE '✓ 创建了 % 对 accepted 好友关系，% 对 pending 好友关系', accepted_count, pending_count;
END $$;

-- 3.6 测试禁止自己加自己为好友
DO $$
BEGIN
    BEGIN
        INSERT INTO friendships (user_id, friend_id, status)
        SELECT id, id, 'accepted' FROM users WHERE username = 'zhangwei';
        RAISE EXCEPTION '✗ 自加好友约束测试失败';
    EXCEPTION 
        WHEN check_violation THEN
            RAISE NOTICE '✓ 自加好友约束测试通过';
    END;
END $$;

-- =====================================================
-- 第四部分：群组测试
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 4. 群组模块测试 ===';
END $$;

-- 4.1 创建群组
INSERT INTO groups (name, description, avatar_url, owner_id, max_members, is_public)
SELECT '技术交流群', '讨论技术问题', '/groups/tech.jpg', id, 200, true
FROM users WHERE username = 'zhangwei'
ON CONFLICT DO NOTHING;

INSERT INTO groups (name, description, avatar_url, owner_id, max_members, is_public)
SELECT '游戏玩家群', '组队开黑', '/groups/game.jpg', id, 100, true
FROM users WHERE username = 'lifang'
ON CONFLICT DO NOTHING;

INSERT INTO groups (name, description, owner_id, max_members, is_public)
SELECT '私密讨论组', '仅限内部', id, 20, false
FROM users WHERE username = 'wangqiang'
ON CONFLICT DO NOTHING;

-- 4.2 添加群成员
INSERT INTO group_members (group_id, user_id, role, nickname)
SELECT g.id, u.id, 
    CASE 
        WHEN u.username = 'zhangwei' AND g.name = '技术交流群' THEN 'owner'
        WHEN u.username = 'lifang' AND g.name = '游戏玩家群' THEN 'owner'
        WHEN u.username = 'wangqiang' AND g.name = '私密讨论组' THEN 'owner'
        ELSE 'member'
    END,
    u.username
FROM groups g, users u
WHERE 
    (g.name = '技术交流群' AND u.username IN ('zhangwei', 'lifang', 'wangqiang')) OR
    (g.name = '游戏玩家群' AND u.username IN ('lifang', 'zhangwei', 'zhaomin', 'chenjing')) OR
    (g.name = '私密讨论组' AND u.username IN ('wangqiang', 'zhaomin'))
ON CONFLICT (group_id, user_id) DO NOTHING;

-- 4.3 设置管理员
UPDATE group_members 
SET role = 'admin' 
WHERE group_id = (SELECT id FROM groups WHERE name = '技术交流群')
  AND user_id = (SELECT id FROM users WHERE username = 'lifang');

-- 验证群组成员
DO $$
DECLARE
    member_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO member_count FROM group_members;
    RAISE NOTICE '✓ 添加了 % 个群组成员', member_count;
END $$;

-- =====================================================
-- 第五部分：消息测试（无 status 字段）
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 5. 消息模块测试 ===';
END $$;

-- 5.1 私聊消息
INSERT INTO messages (from_user_id, to_user_id, message_type, content)
SELECT 
    u1.id, u2.id, 'text', '你好，好久不见！'
FROM users u1, users u2
WHERE u1.username = 'zhangwei' AND u2.username = 'lifang';

INSERT INTO messages (from_user_id, to_user_id, message_type, content)
SELECT 
    u1.id, u2.id, 'text', '是啊，最近怎么样？'
FROM users u1, users u2
WHERE u1.username = 'lifang' AND u2.username = 'zhangwei';

INSERT INTO messages (from_user_id, to_user_id, message_type, content, metadata)
SELECT 
    u1.id, u2.id, 'image', 'https://cdn.example.com/image1.jpg', 
    '{"width": 1920, "height": 1080, "size": "2.5MB"}'
FROM users u1, users u2
WHERE u1.username = 'zhangwei' AND u2.username = 'wangqiang';

INSERT INTO messages (from_user_id, to_user_id, message_type, content)
SELECT
    u1.id, u2.id, 'file', 'project_document.pdf'
FROM users u1, users u2
WHERE u1.username = 'lifang' AND u2.username = 'zhaomin';

-- 5.2 为 wangqiang <-> zhaomin 添加消息（确保好友之间有会话）
INSERT INTO messages (from_user_id, to_user_id, message_type, content)
SELECT
    u1.id, u2.id, 'text', 'Hey, are you coming to the study session?'
FROM users u1, users u2
WHERE u1.username = 'wangqiang' AND u2.username = 'zhaomin';

INSERT INTO messages (from_user_id, to_user_id, message_type, content)
SELECT
    u1.id, u2.id, 'text', 'Yes, see you there!'
FROM users u1, users u2
WHERE u1.username = 'zhaomin' AND u2.username = 'wangqiang';

-- 5.3 群聊消息
INSERT INTO messages (from_user_id, to_group_id, message_type, content)
SELECT 
    u.id, g.id, 'text', '欢迎大家加入技术交流群！'
FROM users u, groups g
WHERE u.username = 'zhangwei' AND g.name = '技术交流群';

INSERT INTO messages (from_user_id, to_group_id, message_type, content)
SELECT 
    u.id, g.id, 'text', '有人一起玩游戏吗？'
FROM users u, groups g
WHERE u.username = 'lifang' AND g.name = '游戏玩家群';

INSERT INTO messages (from_user_id, to_group_id, message_type, content, metadata)
SELECT 
    u.id, g.id, 'audio', 'voice_message_001.mp3',
    '{"duration": 15, "format": "mp3"}'
FROM users u, groups g
WHERE u.username = 'wangqiang' AND g.name = '技术交流群';

INSERT INTO messages (from_user_id, to_group_id, message_type, content)
SELECT 
    u.id, g.id, 'system', '欢迎新成员加入！'
FROM users u, groups g
WHERE u.username = 'zhaomin' AND g.name = '私密讨论组';

-- 5.4 系统消息
INSERT INTO messages (from_user_id, to_user_id, message_type, content)
SELECT 
    u.id, u.id, 'system', '欢迎使用聊天室系统！'
FROM users u
WHERE u.username = 'zhangwei';

-- 验证消息数量
DO $$
DECLARE
    msg_count INTEGER;
    private_count INTEGER;
    group_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO msg_count FROM messages;
    SELECT COUNT(*) INTO private_count FROM messages WHERE to_user_id IS NOT NULL;
    SELECT COUNT(*) INTO group_count FROM messages WHERE to_group_id IS NOT NULL;
    RAISE NOTICE '✓ 发送了 % 条消息（私聊: %, 群聊: %）', msg_count, private_count, group_count;
END $$;

-- =====================================================
-- 第六部分：会话测试（同时考虑发送和接收方向）
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 6. 会话模块测试 ===';
END $$;

-- 清空现有会话数据（避免冲突）
TRUNCATE TABLE conversations;

-- 创建私聊会话（同时考虑用户作为发送方和接收方）
INSERT INTO conversations (user_id, target_user_id, last_message_id, unread_count, last_read_time)
SELECT DISTINCT ON (participant_id, other_participant_id)
    participant_id,
    other_participant_id,
    NULL,
    0,
    NULL
FROM (
    -- 用户作为发送方
    SELECT u.id AS participant_id, m.to_user_id AS other_participant_id
    FROM users u
    JOIN messages m ON u.id = m.from_user_id AND m.to_user_id IS NOT NULL
    UNION
    -- 用户作为接收方
    SELECT u.id AS participant_id, m.from_user_id AS other_participant_id
    FROM users u
    JOIN messages m ON u.id = m.to_user_id AND m.from_user_id IS NOT NULL
) AS all_participants
WHERE participant_id != other_participant_id
ON CONFLICT (user_id, target_user_id) DO NOTHING;

-- 更新每个会话的 last_message_id 和 unread_count（简单起见，设 last_message_id 为最新消息 id，unread_count = 0）
UPDATE conversations c
SET last_message_id = (
    SELECT m.id
    FROM messages m
    WHERE (m.from_user_id = c.user_id AND m.to_user_id = c.target_user_id)
       OR (m.from_user_id = c.target_user_id AND m.to_user_id = c.user_id)
    ORDER BY m.created_at DESC
    LIMIT 1
),
unread_count = 0
WHERE c.target_user_id IS NOT NULL;

-- 创建群聊会话
INSERT INTO conversations (user_id, target_group_id, last_message_id, unread_count)
SELECT DISTINCT ON (gm.user_id, gm.group_id)
    gm.user_id,
    gm.group_id,
    NULL,
    0
FROM group_members gm
ON CONFLICT (user_id, target_group_id) DO NOTHING;

-- 更新群聊会话的 last_message_id（取该群最新一条消息）
UPDATE conversations c
SET last_message_id = (
    SELECT m.id
    FROM messages m
    WHERE m.to_group_id = c.target_group_id
    ORDER BY m.created_at DESC
    LIMIT 1
)
WHERE c.target_group_id IS NOT NULL;

-- 验证会话数量
DO $$
DECLARE
    conv_count INTEGER;
    private_count INTEGER;
    group_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO conv_count FROM conversations;
    SELECT COUNT(*) INTO private_count FROM conversations WHERE target_user_id IS NOT NULL;
    SELECT COUNT(*) INTO group_count FROM conversations WHERE target_group_id IS NOT NULL;
    RAISE NOTICE '✓ 创建了 % 个会话（私聊: %, 群聊: %）', conv_count, private_count, group_count;
END $$;

-- =====================================================
-- 第七部分：文件测试
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 7. 文件模块测试 ===';
END $$;

INSERT INTO files (user_id, file_name, file_path, file_size, mime_type, md5_hash, upload_status, expires_at)
SELECT 
    id, 
    'profile_avatar.jpg', 
    '/uploads/profile_avatar.jpg', 
    1024000, 
    'image/jpeg',
    md5('test file content'),
    'completed',
    CURRENT_TIMESTAMP + interval '30 days'
FROM users WHERE username = 'zhangwei'
ON CONFLICT DO NOTHING;

INSERT INTO files (user_id, file_name, file_path, file_size, mime_type, md5_hash, upload_status)
SELECT 
    id, 
    'meeting_recording.mp4', 
    '/uploads/meeting_recording.mp4', 
    52428800, 
    'video/mp4',
    md5('video content'),
    'uploading'
FROM users WHERE username = 'lifang'
ON CONFLICT DO NOTHING;

INSERT INTO files (user_id, file_name, file_path, file_size, mime_type, md5_hash, upload_status)
SELECT 
    id, 
    'data_export.csv', 
    '/exports/data_export.csv', 
    512000, 
    'text/csv',
    md5('csv data'),
    'completed'
FROM users WHERE username = 'wangqiang'
ON CONFLICT DO NOTHING;

DO $$
DECLARE
    file_count INTEGER;
    completed_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO file_count FROM files;
    SELECT COUNT(*) INTO completed_count FROM files WHERE upload_status = 'completed';
    RAISE NOTICE '✓ 上传了 % 个文件（已完成: %）', file_count, completed_count;
END $$;

-- =====================================================
-- 第八部分：复杂查询测试
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 8. 复杂查询测试 ===';
END $$;

-- 8.1 查询用户的好友列表（包括 pending 和 accepted）
SELECT '用户好友列表查询' as test_name;
SELECT 
    u.username as user,
    string_agg(CONCAT(f.username, ' (', fs.status, ')'), ', ') as friends
FROM users u
LEFT JOIN friendships fs ON u.id = fs.user_id
LEFT JOIN users f ON fs.friend_id = f.id
WHERE u.username IN ('zhangwei', 'lifang', 'chenjing')
GROUP BY u.username
ORDER BY u.username;

-- 8.2 查询用户所在的群组
SELECT '用户群组查询' as test_name;
SELECT 
    u.username as user,
    string_agg(g.name, ', ') as groups
FROM users u
LEFT JOIN group_members gm ON u.id = gm.user_id
LEFT JOIN groups g ON gm.group_id = g.id
WHERE u.username IN ('zhangwei', 'lifang', 'wangqiang')
GROUP BY u.username
ORDER BY u.username;

-- 8.3 查询群组的成员列表
SELECT '群组成员查询' as test_name;
SELECT 
    g.name as group_name,
    string_agg(u.username, ', ' ORDER BY 
        CASE gm.role 
            WHEN 'owner' THEN 1 
            WHEN 'admin' THEN 2 
            ELSE 3 
        END) as members
FROM groups g
LEFT JOIN group_members gm ON g.id = gm.group_id
LEFT JOIN users u ON gm.user_id = u.id
WHERE g.name IN ('技术交流群', '游戏玩家群')
GROUP BY g.name;

-- 8.4 查询未读消息统计（基于 conversations.unread_count）
SELECT '未读消息统计' as test_name;
SELECT 
    u.username,
    COUNT(CASE WHEN c.target_user_id IS NOT NULL AND c.unread_count > 0 THEN 1 END) as unread_private,
    COUNT(CASE WHEN c.target_group_id IS NOT NULL AND c.unread_count > 0 THEN 1 END) as unread_group
FROM users u
LEFT JOIN conversations c ON u.id = c.user_id
GROUP BY u.username
ORDER BY u.username;

-- 8.5 查询最近的聊天记录
SELECT '最近聊天记录' as test_name;
SELECT 
    u_sender.username as from_user,
    CASE 
        WHEN m.to_user_id IS NOT NULL THEN u_receiver.username 
        ELSE g.name 
    END as target,
    CASE 
        WHEN m.to_user_id IS NOT NULL THEN 'private' 
        ELSE 'group' 
    END as chat_type,
    m.message_type,
    LEFT(m.content, 30) as content_preview,
    m.created_at
FROM messages m
JOIN users u_sender ON m.from_user_id = u_sender.id
LEFT JOIN users u_receiver ON m.to_user_id = u_receiver.id
LEFT JOIN groups g ON m.to_group_id = g.id
ORDER BY m.created_at DESC
LIMIT 10;

-- 8.6 统计活跃用户
SELECT '活跃用户统计' as test_name;
SELECT 
    u.username,
    COUNT(DISTINCT m.id) as total_messages,
    COUNT(DISTINCT CASE WHEN m.to_user_id IS NOT NULL THEN m.to_user_id END) as private_contacts,
    COUNT(DISTINCT m.to_group_id) as groups_joined
FROM users u
LEFT JOIN messages m ON u.id = m.from_user_id
GROUP BY u.username
ORDER BY total_messages DESC;

-- =====================================================
-- 第九部分：边界条件和异常测试
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 9. 边界条件和异常测试 ===';
END $$;

-- 9.1 测试消息没有接收者（应该失败）
DO $$
BEGIN
    BEGIN
        INSERT INTO messages (from_user_id, message_type, content) 
        VALUES (1, 'text', 'No target');
        RAISE EXCEPTION '✗ 消息目标约束测试失败';
    EXCEPTION 
        WHEN check_violation THEN
            RAISE NOTICE '✓ 消息必须有接收者的约束测试通过';
    END;
END $$;

-- 9.2 测试重复好友请求（应该失败）
DO $$
DECLARE
    user1_id BIGINT;
    user2_id BIGINT;
BEGIN
    SELECT id INTO user1_id FROM users WHERE username = 'zhangwei' LIMIT 1;
    SELECT id INTO user2_id FROM users WHERE username = 'lifang' LIMIT 1;
    
    BEGIN
        INSERT INTO friend_requests (from_user_id, to_user_id, status, message)
        VALUES (user1_id, user2_id, 'pending', 'Duplicate request');
        RAISE EXCEPTION '✗ 重复好友请求约束测试失败';
    EXCEPTION 
        WHEN unique_violation THEN
            RAISE NOTICE '✓ 重复好友请求约束测试通过';
    END;
END $$;

-- 9.3 测试无效状态值（应该失败）
DO $$
BEGIN
    BEGIN
        INSERT INTO users (username, email, password_hash, status) 
        VALUES ('invalidstatus', 'invalid@example.com', crypt('test', gen_salt('bf')), 'invalid_status');
        RAISE EXCEPTION '✗ 状态枚举约束测试失败';
    EXCEPTION 
        WHEN check_violation THEN
            RAISE NOTICE '✓ 状态枚举约束测试通过';
    END;
END $$;

-- =====================================================
-- 第十部分：数据完整性测试
-- =====================================================

DO $$
BEGIN
    RAISE NOTICE '';
    RAISE NOTICE '=== 10. 数据完整性测试 ===';
END $$;

-- 10.1 检查外键约束
SELECT 
    '外键约束检查' as check_name,
    (SELECT COUNT(*) FROM user_settings us 
     LEFT JOIN users u ON us.user_id = u.id 
     WHERE u.id IS NULL) as orphaned_settings,
    (SELECT COUNT(*) FROM friendships f 
     LEFT JOIN users u1 ON f.user_id = u1.id 
     LEFT JOIN users u2 ON f.friend_id = u2.id 
     WHERE u1.id IS NULL OR u2.id IS NULL) as orphaned_friendships,
    (SELECT COUNT(*) FROM group_members gm 
     LEFT JOIN groups g ON gm.group_id = g.id 
     LEFT JOIN users u ON gm.user_id = u.id 
     WHERE g.id IS NULL OR u.id IS NULL) as orphaned_members;

-- 10.2 测试触发器效果
DO $$
DECLARE
    old_updated_at TIMESTAMP;
    new_updated_at TIMESTAMP;
    user_id_var BIGINT;
BEGIN
    SELECT id, updated_at INTO user_id_var, old_updated_at FROM users WHERE username = 'zhangwei';
    
    UPDATE users SET avatar_url = '/avatars/updated.jpg' WHERE id = user_id_var;
    
    SELECT updated_at INTO new_updated_at FROM users WHERE id = user_id_var;
    
    IF new_updated_at > old_updated_at THEN
        RAISE NOTICE '✓ updated_at 触发器工作正常（自动更新）';
    ELSE
        RAISE NOTICE '✗ updated_at 触发器未正常工作';
    END IF;
END $$;

-- =====================================================
-- 测试结果汇总
-- =====================================================

DO $$
DECLARE
    user_count INTEGER;
    friend_accepted INTEGER;
    friend_pending INTEGER;
    group_count INTEGER;
    member_count INTEGER;
    message_count INTEGER;
    file_count INTEGER;
    conv_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO user_count FROM users;
    SELECT COUNT(*) INTO friend_accepted FROM friendships WHERE status = 'accepted';
    SELECT COUNT(*) INTO friend_pending FROM friendships WHERE status = 'pending';
    SELECT COUNT(*) INTO group_count FROM groups;
    SELECT COUNT(*) INTO member_count FROM group_members;
    SELECT COUNT(*) INTO message_count FROM messages;
    SELECT COUNT(*) INTO file_count FROM files;
    SELECT COUNT(*) INTO conv_count FROM conversations;
    
    RAISE NOTICE '';
    RAISE NOTICE '==========================================';
    RAISE NOTICE '测试完成 - 数据统计汇总';
    RAISE NOTICE '==========================================';
    RAISE NOTICE '用户数量: %', user_count;
    RAISE NOTICE '好友关系(accepted): %', friend_accepted;
    RAISE NOTICE '好友关系(pending): %', friend_pending;
    RAISE NOTICE '群组数量: %', group_count;
    RAISE NOTICE '群组成员: %', member_count;
    RAISE NOTICE '消息总数: %', message_count;
    RAISE NOTICE '文件记录: %', file_count;
    RAISE NOTICE '会话数量: %', conv_count;
    RAISE NOTICE '==========================================';
    RAISE NOTICE '所有测试用例执行完毕！';
    RAISE NOTICE '==========================================';
END $$;