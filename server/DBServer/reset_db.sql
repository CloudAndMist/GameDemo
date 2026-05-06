-- ========== reset_db.sql ==========
-- V0.4.5 GameDemo 数据库重置脚本
-- 一账户一角色设计
-- 注意: 位置信息由 SceneServer 管理，不存储在 DB
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

-- 2. 重置账号表 (V0.4.5: roles 已合并到 accounts，位置由 SceneServer 管理)
TRUNCATE TABLE accounts;

-- ============================================
-- 验证数据（仅查询，不插入）
-- ============================================
SELECT '=== Accounts ===' as '';
SELECT * FROM accounts;

SELECT '=== Player Online ===' as '';
SELECT * FROM player_online;

SELECT '=== Reset Complete ===' as '';
SELECT CONCAT('Total accounts: ', COUNT(*)) as summary FROM accounts;
