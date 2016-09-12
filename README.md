# stepic_converter

Программа сервис для конвертирования аудио. В директории converter находится серверная часть, в директории converter_client клиентская.
Для сборки серверной части необходимы 
- libev
- libavcodec-ffmpeg libavutil-ffmpeg libavformat-ffmpeg libavresample-ffmpeg libswresample-ffmpeg (версия 2.8.6)
- protobuf (версия 2.6)

Для клиентской части необходим Qt 5.7 и protobuf


Сборка происходим с помощью qmake.
