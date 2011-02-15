-- Gates (Horde)
REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `ScriptName`, `WDBVerified`) VALUES (203710, 0, 6676, 'Portcullis', '', '', '', 0, 36, 0.9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1);

-- Gates (Alliance)
REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `ScriptName`, `WDBVerified`) VALUES (206654, 0, 10123, 'Doodad_TwinPeaks_Dwarven_Gate_02', '', '', '', 0, 36, 1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1);
REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `ScriptName`, `WDBVerified`) VALUES (206653, 0, 10122, 'Doodad_TwinPeaks_Dwarven_Gate_01', '', '', '', 0, 36, 1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1);
REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `ScriptName`, `WDBVerified`) VALUES (206655, 0, 10124, 'Doodad_TwinPeaks_Dwarven_Gate_03', '', '', '', 0, 36, 1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1);

-- Flags
UPDATE gameobject_template SET name='Horde Flag' WHERE entry IN (179831,179786);
UPDATE gameobject_template SET name='Alliance Flag' WHERE entry IN (179830,179785);

-- Cleanup
delete from gameobject where map=726;
delete from creature where map=726;

-- Texts
INSERT INTO `trinity_string` VALUES ('820', 'The battle in Twin Peaks begins in 2 minutes.', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('821', 'The battle in Twin Peaks begins in 1 minute.', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('822', 'The battle in Twin Peaks begins in 30 seconds. Prepare yourselves!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('823', 'Let the battle in Twin Peaks begin!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('824', '$n captured the Horde flag!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('825', '$n captured the Alliance flag!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('826', 'The Horde flag was dropped by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('827', 'The Alliance Flag was dropped by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('828', 'The Alliance Flag was returned to its base by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('829', 'The Horde flag was returned to its base by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('830', 'The Horde flag was picked up by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('831', 'The Alliance Flag was picked up by $n!', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('832', 'The flags are now placed at their bases.', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('833', 'The Alliance flag is now placed at its base.', null, null, null, null, null, null, null, null);
INSERT INTO `trinity_string` VALUES ('834', 'The Horde flag is now placed at its base.', null, null, null, null, null, null, null, null);
