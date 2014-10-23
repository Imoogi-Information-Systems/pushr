pushr
=====

A C Push Notification library/client for Android (at the moment).

pushr is written as a sort of service to be invoked when a new message is added
to the queue. The queue can be either files in a folder or rows in a MySQL table 
(see below for the create command).

_Getting Started_

1) Download source
2) Edit the settings.h file. There you should set up your API key and folders
and MySQL login information
3) Compile (run *make*)
4) Add a message to the queue of your choice and invoke pushr(*pushr files* for
the filesystem queue, *pushr mysql* for the DB queue)
5) pushr will process ALL the messages in the queue yet to be processes. pushr
makes sure it has only one process, so feel free to invoke it as often as you need.


_Create command for the MySQL table:_

CREATE TABLE `PushrMessages` (
  `Id` bigint NOT NULL AUTO_INCREMENT PRIMARY KEY,
  `RegistrationIds` text COLLATE 'utf8mb4_unicode_ci' NOT NULL,
  `NotificationKey` varchar(512) COLLATE 'utf8mb4_unicode_ci' NOT NULL,
  `Data` text COLLATE 'utf8mb4_unicode_ci' NOT NULL,
  `CollapseKey` varchar(512) COLLATE 'utf8mb4_unicode_ci' NOT NULL,
  `DelayWhileIdle` tinyint(1) NOT NULL DEFAULT '0',
  `TimeToLive` bigint NOT NULL,
  `PackageName` varchar(512) COLLATE 'utf8mb4_unicode_ci' NOT NULL,
  `DryRun` tinyint(1) NOT NULL DEFAULT '0',
  `IsSent` tinyint(1) NOT NULL DEFAULT '0',
  `IsError` tinyint(1) NOT NULL DEFAULT '0',
  `Timestamp` datetime NOT NULL,
  `ServerResponse` text COLLATE 'utf8mb4_unicode_ci' NOT NULL
) COMMENT='' ENGINE='InnoDB' COLLATE 'utf8mb4_unicode_ci';

_Messages_

1) Messages saved as file are delimited with the new line character (\n).
2) Be sure to send a valid JSON object as your data.
3) For the DB, you have to fill in the RegistrationIds and the Data fields. The
results will be saved in the fields IsSent, IsError, Timestamp and ServerResponse.
4) The order of the fields in the file format:
- Registration Ids (strings seperated by commas, e.g.: "1", "2", "222")
- Notification key
- The data JSON object
- The collapse key
- Delay if idle (true/false)
- Time to live (number)
- Package name (can be an empty string)
- Dry run (true/false)
5) As with the DB format, only the registration ids and the data are required. 
Use empty lines to skip fields (or stop after the data if you don't care for the
other fields)
6) Information about what the fields mean can be found @ http://developer.android.com/google/gcm/server.html

_Getting in touch_

The first version of pushr was written by Itay Avtalyon from Imoogi Information Systems.
I can be reached at itay@imoogi.com.

Feel free to drop me a line and I'll help if I can.

Have fun and happy pushing!
