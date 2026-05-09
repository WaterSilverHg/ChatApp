-- 终止所有连接到 chatroom 数据库的连接
SELECT pg_terminate_backend(pid)
FROM pg_stat_activity
WHERE datname = 'chatroom' AND pid <> pg_backend_pid();

-- 删除并创建数据库
DROP DATABASE IF EXISTS chatroom;
CREATE DATABASE chatroom
    WITH 
    OWNER = postgres
    ENCODING = 'UTF8'
    CONNECTION LIMIT = -1;