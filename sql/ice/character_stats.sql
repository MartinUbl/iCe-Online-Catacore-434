
ALTER TABLE `character_stats` ADD COLUMN `resilience` int(10) unsigned NOT NULL DEFAULT '0' AFTER `spellPower`;
ALTER TABLE `character_stats` ADD COLUMN `achievPoints` int(10) unsigned NOT NULL DEFAULT '0' AFTER `resilience`;
