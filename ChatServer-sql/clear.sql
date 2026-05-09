-- =====================================================
-- clear.sql
-- 删除数据库中所有业务表（包括表结构），以便重新执行 psqlinit2.sql 初始化
-- 注意：此操作不可逆，请谨慎使用！
-- =====================================================

BEGIN;

-- 删除表（顺序按外键依赖，先删子表，再删父表）
DROP TABLE IF EXISTS conversations;
DROP TABLE IF EXISTS message_reads;
DROP TABLE IF EXISTS offline_messages;
DROP TABLE IF EXISTS messages;
DROP TABLE IF EXISTS group_members;
DROP TABLE IF EXISTS groups;
DROP TABLE IF EXISTS friendships;
DROP TABLE IF EXISTS friend_requests;
DROP TABLE IF EXISTS files;
DROP TABLE IF EXISTS user_settings;
DROP TABLE IF EXISTS users;

COMMIT;

-- 检查核心表是否已删除（如果表不存在，pg_tables 中无记录）
DO $$
DECLARE
    table_exists BOOLEAN;
    table_list TEXT[] := ARRAY['users', 'conversations', 'messages', 'groups'];
    t TEXT;
BEGIN
    RAISE NOTICE '==========================================';
    RAISE NOTICE '验证表删除结果（应全部为 false）';
    RAISE NOTICE '==========================================';
    FOREACH t IN ARRAY table_list
    LOOP
        SELECT EXISTS (
            SELECT 1 FROM pg_tables WHERE tablename = t AND schemaname = 'public'
        ) INTO table_exists;
        RAISE NOTICE '表 % 是否存在: %', t, table_exists;
    END LOOP;
    RAISE NOTICE '==========================================';
    RAISE NOTICE '所有业务表已删除，可执行 psqlinit2.sql 重新建表。';
    RAISE NOTICE '==========================================';
END $$;

SELECT 'All tables have been dropped successfully!' AS status;