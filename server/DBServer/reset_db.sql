-- ========== reset_db.sql ==========
-- GameDemo 数据库重置脚本
-- 用于每次测试前初始化数据库
-- 注意: MySQL 容器名称为 tars-mysql，密码为 123456，使用时连接命令示例:
--   docker exec -i tars-mysql mysql -u root -p123456 < reset_db.sql
-- 或在容器内执行:
--   mysql -u root -p123456

-- 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS gamedemo DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE gamedemo;

-- ============================================
-- 重置表：删除所有数据并重置自增ID
-- ============================================

-- 1. 重置玩家在线状态表（先清空，因为依赖其他表）
TRUNCATE TABLE player_online;

-- 2. 重置角色表（先清空，因为依赖账号表）
TRUNCATE TABLE roles;

-- 3. 重置账号表
TRUNCATE TABLE accounts;

-- ============================================
-- 验证数据（仅查询，不插入）
-- ============================================
SELECT '=== Accounts ===' as '';
SELECT * FROM accounts;

SELECT '=== Roles ===' as '';
SELECT * FROM roles;

SELECT '=== Reset Complete ===' as '';
SELECT CONCAT('Total accounts: ', COUNT(*)) as summary FROM accounts;
SELECT CONCAT('Total roles: ', COUNT(*)) as summary FROM roles;
