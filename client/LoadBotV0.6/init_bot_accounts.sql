-- ==========================================
-- V0.6 压测账号预创建脚本
-- 执行方式: docker exec -i tars-mysql mysql -uroot -p123456 gamedemo < init_bot_accounts.sql
-- ==========================================

USE gamedemo;

-- 清理旧压测账号 (username 以 'bot_' 开头)
DELETE FROM accounts WHERE username LIKE 'bot_%';

-- 使用存储过程批量生成 1000 个压测账号 (注册 + 创角一步到位)
-- 一账户一角色设计: accountId = playerId (自增 id 即 playerId)
-- 职业分配: 1战士 2法师 3猎人，均匀轮转
DELIMITER //
DROP PROCEDURE IF EXISTS create_bot_accounts//
CREATE PROCEDURE create_bot_accounts()
BEGIN
    DECLARE i INT DEFAULT 1;
    DECLARE v_job INT;
    DECLARE v_name VARCHAR(64);
    WHILE i <= 1000 DO
        -- 职业轮转: 1→2→3→1→2→3...
        SET v_job = ((i - 1) MOD 3) + 1;
        SET v_name = CONCAT('Bot', LPAD(i, 4, '0'));
        
        INSERT INTO accounts (username, password, player_name, job, level, exp, hp, max_hp, mp, max_mp, create_time, last_login_time)
        VALUES (
            CONCAT('bot_', LPAD(i, 4, '0')),  -- username: bot_0001 ~ bot_1000
            'bot123',                          -- password: 统一密码
            v_name,                            -- player_name: Bot0001 ~ Bot1000
            v_job,                             -- job: 1战士/2法师/3猎人 均匀分配
            1, 0, 100, 100, 50, 50,           -- 默认属性
            UNIX_TIMESTAMP() * 1000,           -- create_time
            0                                  -- last_login_time
        );
        
        SET i = i + 1;
    END WHILE;
END//
DELIMITER ;

-- 执行
CALL create_bot_accounts();

-- 验证
SELECT COUNT(*) AS total_accounts FROM accounts WHERE username LIKE 'bot_%';
SELECT job, COUNT(*) AS cnt FROM accounts WHERE username LIKE 'bot_%' GROUP BY job;

-- 清理存储过程
DROP PROCEDURE IF EXISTS create_bot_accounts;
