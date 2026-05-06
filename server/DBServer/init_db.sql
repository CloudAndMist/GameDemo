CREATE DATABASE IF NOT EXISTS gamedemo CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE gamedemo;

-- 账号表 V0.4.5 (支持 username + 角色)
DROP TABLE IF EXISTS accounts;
CREATE TABLE accounts (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) NOT NULL UNIQUE COMMENT '用户名',
    password VARCHAR(128) NOT NULL COMMENT '密码(加密)',
    player_name VARCHAR(64) DEFAULT '' COMMENT '角色名',
    job INT DEFAULT 0 COMMENT '职业: 1战士 2法师 3猎人',
    level INT DEFAULT 1 COMMENT '等级',
    exp BIGINT DEFAULT 0 COMMENT '经验值',
    hp INT DEFAULT 100 COMMENT '血量',
    max_hp INT DEFAULT 100 COMMENT '最大血量',
    mp INT DEFAULT 50 COMMENT '魔法值',
    max_mp INT DEFAULT 50 COMMENT '最大魔法值',
    create_time BIGINT DEFAULT 0 COMMENT '创建时间戳',
    last_login_time BIGINT DEFAULT 0 COMMENT '最后登录时间戳',
    INDEX idx_username (username)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='账号表';

-- 在线状态表
DROP TABLE IF EXISTS player_online;
CREATE TABLE player_online (
    player_id BIGINT PRIMARY KEY COMMENT '玩家ID(关联accounts.id)',
    server_id INT DEFAULT 0 COMMENT '所在服务器ID',
    scene_id INT DEFAULT 0 COMMENT '所在场景ID',
    online_time BIGINT DEFAULT 0 COMMENT '上线时间戳',
    INDEX idx_server (server_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='在线状态表';
