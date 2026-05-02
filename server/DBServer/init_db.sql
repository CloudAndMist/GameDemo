-- ========== init_db.sql ==========
-- GameDemo 数据库初始化脚本

-- 创建数据库
CREATE DATABASE IF NOT EXISTS gamedemo DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE gamedemo;

-- 账号表
CREATE TABLE IF NOT EXISTS `accounts` (
    `id` BIGINT PRIMARY KEY AUTO_INCREMENT,
    `qq_number` VARCHAR(20) UNIQUE NOT NULL COMMENT 'QQ号码',
    `password` VARCHAR(128) NOT NULL COMMENT '密码(建议加密存储)',
    `create_time` BIGINT COMMENT '创建时间(毫秒时间戳)',
    `update_time` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_qq_number (`qq_number`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='账号表';

-- 角色表
CREATE TABLE IF NOT EXISTS `roles` (
    `id` BIGINT PRIMARY KEY AUTO_INCREMENT,
    `account_id` BIGINT NOT NULL COMMENT '所属账号ID',
    `role_name` VARCHAR(64) NOT NULL COMMENT '角色名',
    `job` INT DEFAULT 0 COMMENT '职业',
    `level` INT DEFAULT 1 COMMENT '等级',
    `exp` BIGINT DEFAULT 0 COMMENT '经验',
    `hp` INT DEFAULT 100 COMMENT '生命值',
    `mp` INT DEFAULT 50 COMMENT '魔法值',
    `x` FLOAT DEFAULT 0 COMMENT 'X坐标',
    `y` FLOAT DEFAULT 0 COMMENT 'Y坐标',
    `z` FLOAT DEFAULT 0 COMMENT 'Z坐标',
    `scene_id` INT DEFAULT 1 COMMENT '所在场景ID',
    `create_time` BIGINT COMMENT '创建时间(毫秒时间戳)',
    `last_login_time` BIGINT COMMENT '最后登录时间(毫秒时间戳)',
    `update_time` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_account_id (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色表';

-- 玩家在线状态表 (可选，用于断线处理)
CREATE TABLE IF NOT EXISTS `player_online` (
    `player_id` BIGINT PRIMARY KEY COMMENT '玩家ID',
    `scene_id` INT DEFAULT 0 COMMENT '所在场景ID',
    `lobby_server` VARCHAR(64) DEFAULT '' COMMENT '连接的LobbyServer',
    `login_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '登录时间',
    `last_heartbeat` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后心跳时间',
    INDEX idx_scene_id (`scene_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='玩家在线状态表';
