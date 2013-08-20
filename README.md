
Usage
-----
    usage: ecp [OPTION] SOURCE... DESTINATION
    
    OPTION can be:
     -h=HASHALGO      decide which hash algorithm should be used.
                        HASHALGO can be:
                          NONE, MD5, SHA1, SHA224, SHA256, SHA384, SHA512.
                        Default is MD5
     -d               create empty directorys
     -d               create empty directories
     -v               verify the checksum after copy (additionally)
     --version        shows version number


What It Does
------------
This tools allows to copy one or more files calculating their checksum
on the fly as it transfers their data. The checksums are printed to
the standard output if something goes wrong.

It works similar to the cp command, it allows you to copy multiple files
into a directory, or whole directory structures.

How It Works
------------
1. During the copy process the checksum will be calculated.
2. After the copy process the calculated checksum will be compared to the new file checksum
3. (Optional use `-v`)  The checksum of the source file will be recalculated (if Step 1 has read errors) and compared to the new file checksum.

Using all 3 Steps, this is simply a replacement for:
    cp source_file destination_file
    md5sum sourcefile
    md5sum destination_file
(with multiple files, good for backups)

Using 2 Steps, you have a simple copy command and you verify that the data transfer is succeeded without problems.


    
TODO
----
* delete files after successfull copied, aka move files
* delete files after successful copied, aka move files
* show start time, end time, time taken
* crc32



CHANGELOG
---------

* 1.02:
    * Added verify after copy
    * spelling
    * fsync() and syncfs() if possible