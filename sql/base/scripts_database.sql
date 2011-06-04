CREATE TABLE `gameobject_teleport_cond` (

  `id` int(11) NOT NULL DEFAULT '0',

  `guid` int(11) NOT NULL DEFAULT '0',

  `quest` int(11) NOT NULL DEFAULT '0',

  `race` int(8) NOT NULL DEFAULT '0',

  `class` int(8) NOT NULL DEFAULT '0',

  `level` int(8) NOT NULL DEFAULT '0',

  `item` int(11) NOT NULL DEFAULT '0',

  `spell` int(11) NOT NULL DEFAULT '0',

  `x` float NOT NULL,
  `y` float NOT NULL,

  `z` float NOT NULL,
  `o` float NOT NULL DEFAULT '0',

  `map` int(11) NOT NULL DEFAULT '0',

PRIMARY KEY (`id`,`guid`,`quest`,`race`,`class`,`level`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
