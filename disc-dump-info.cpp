#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>
#include <cdio/cdio.h>
#include <cdio/device.h>
#include <cdio/audio.h>
#include <cdio/cd_types.h>
#include <cdio/cdtext.h>
#include <openssl/evp.h>

const char* get_audio_status_desc(uint8_t status)
{
    switch (status)
    {
        case 0x00:
            return "No audio status";
        case 0x11:
            return "Audio playing";
        case 0x12:
            return "Audio paused";
        default:
            return "Unknown status";
    }
}

const char* get_address_desc(uint8_t address)
{
    switch (address)
    {
        case 0x0:
            return "Track Number";
        case 0x1:
            return "Absolute Time";
        case 0x2:
            return "Media Catalog Number";
        case 0x3:
            return "ISRC";
        default:
            return "Other";
    }
}

void print_control_flags(uint8_t control)
{
    printf("Control Flags:\n");
    printf("Data Track: %s\n", (control & 0x4) ? "Yes" : "No");
    printf("Copy Permitted: %s\n", (control & 0x2) ? "Yes" : "No");
    printf("Pre-emphasis: %s\n", (control & 0x1) ? "Yes" : "No");
}

void print_cdio_subchannel(const cdio_subchannel_t& subchannel)
{
    printf("Format: %u\n", subchannel.format);

    printf("Audio Status: %u (%s)\n", subchannel.audio_status,
            get_audio_status_desc(subchannel.audio_status));

    printf("Address: %u (%s)\n", subchannel.address,
            get_address_desc(subchannel.address));

    printf("Control: %u\n", subchannel.control);
    print_control_flags(subchannel.control);

    printf("Track: %u\n", subchannel.track);
    printf("Index: %u\n", subchannel.index);

    char * relative_addr_str = cdio_msf_to_str(&subchannel.abs_addr);
    printf("Absolute Address: %s\n", relative_addr_str);
    free(relative_addr_str);

    char * absolute_addr_str = cdio_msf_to_str(&subchannel.rel_addr);
    printf("Relative Address: %s\n\n", absolute_addr_str);
    free(absolute_addr_str);
}


void print_formatted_secs(const char *msg, const uint32_t secs)
{
    printf("%s: %02u:%02u:%02u\n", msg, secs / 3600, (secs % 3600) / 60,
            secs % 60);
}

void print_filesystem_name(const cdio_fs_t fs_type)
{

    constexpr const char * cdio_fs_names[] =
    {
        "Unknown",
        "CDIO_FS_AUDIO",
        "CDIO_FS_HIGH_SIERRA",
        "CDIO_FS_ISO_9660",
        "CDIO_FS_INTERACTIVE",
        "CDIO_FS_HFS",
        "CDIO_FS_UFS",
        "CDIO_FS_EXT2",
        "CDIO_FS_ISO_HFS",
        "CDIO_FS_ISO_9660_INTERACTIVE",
        "CDIO_FS_3DO",
        "CDIO_FS_XISO",
        "CDIO_FS_UDFX",
        "CDIO_FS_UDF",
        "CDIO_FS_ISO_UDF"
    };


    if (fs_type >= 1 && fs_type <= 14)
    {
        printf("Filesystem Type: %s\n", cdio_fs_names[fs_type]);
    }
    else
    {
        printf("Filesystem Type: Unknown\n");
    }
}

void print_disc_format(const cdio_fs_cap_t disc_format)
{
    std::map<cdio_fs_cap_t, const char *> cdio_fs_map =
    {
        {cdio_fs_cap_t::CDIO_FS_ANAL_XA, "CDIO_FS_ANAL_XA"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_MULTISESSION, "CDIO_FS_ANAL_MULTISESSION"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_PHOTO_CD, "CDIO_FS_ANAL_PHOTO_CD"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_HIDDEN_TRACK, "CDIO_FS_ANAL_HIDDEN_TRACK"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_CDTV, "CDIO_FS_ANAL_CDTV"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_BOOTABLE, "CDIO_FS_ANAL_BOOTABLE"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_VIDEOCD, "CDIO_FS_ANAL_VIDEOCD"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_ROCKRIDGE, "CDIO_FS_ANAL_ROCKRIDGE"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_JOLIET, "CDIO_FS_ANAL_JOLIET"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_SVCD, "CDIO_FS_ANAL_SVCD"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_CVD, "CDIO_FS_ANAL_CVD"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_XISO, "CDIO_FS_ANAL_XISO"},
        {cdio_fs_cap_t::CDIO_FS_ANAL_ISO9660_ANY, "CDIO_FS_ANAL_ISO9660_ANY"}
    };

    if(cdio_fs_map.find(disc_format) != cdio_fs_map.cend())
    {
        printf("Disc format: %s\n", cdio_fs_map[disc_format]);
    }
}

void print_cdio_fs_info(const cdio_iso_analysis_t& fs_info, const bool is_udf, const bool is_joliet)
{
    if(is_joliet)
    {
        printf("Joliet Level: %u\n", fs_info.joliet_level);
    }

    if(fs_info.iso_label != nullptr && fs_info.iso_label[0] != '\0')
    {
        printf("ISO Label: %s\n", fs_info.iso_label);
    }

    if(fs_info.isofs_size)
    {
        printf("ISO Filesystem Size: %u\n", fs_info.isofs_size);
    }

    if(is_udf)
    {
        printf("UDF Version Major: %u\n", fs_info.UDFVerMajor);
        printf("UDF Version Minor: %u\n", fs_info.UDFVerMinor);
    }
}

int main()
{
     CdIo_t *cdio = cdio_open(nullptr, DRIVER_DEVICE);
    if(cdio == nullptr)
    {
        printf("[Error] Failed to open cdio drive");
    }

    cdio_iso_analysis_t cd_analysis{};
    cdio_fs_anal_t fs_guessed_data = cdio_guess_cd_type(cdio, 0, cdio_get_first_track_num(cdio), &cd_analysis);
    const cdio_fs_t fs_format = static_cast<cdio_fs_t>(CDIO_FSTYPE(fs_guessed_data));
    print_filesystem_name(fs_format);
    const bool is_udf = (fs_format == CDIO_FS_UDF) || (fs_format == CDIO_FS_ISO_UDF);
    const bool is_joliet = (fs_guessed_data == CDIO_FS_ANAL_JOLIET);
    print_cdio_fs_info(cd_analysis, is_udf, is_joliet);
    print_disc_format(static_cast<cdio_fs_cap_t>(fs_guessed_data));
    puts("");

    if(fs_format == cdio_fs_t::CDIO_FS_AUDIO)
    {
        cdio_audio_volume_t driver_audio_volume{};
        const driver_return_code_t audio_volume_res = cdio_audio_get_volume(cdio, &driver_audio_volume);
        if(audio_volume_res == driver_return_code_t::DRIVER_OP_SUCCESS)
        {
            constexpr const char * channel_id_str[4] = {"front left", "front right", "rear left", "rear right"};

            for(uint8_t channel_index = 0; channel_index < sizeof(driver_audio_volume.level); channel_index++)
            {
                printf("Channel %s volume: %u\n", channel_id_str[channel_index],
                        driver_audio_volume.level[channel_index]);
            }
        }
        else
        {
            printf("Failed to obtain CD driver audio volume, err: %d\n", audio_volume_res);
        }

        const uint8_t audio_cd_track_count = cdio_get_num_tracks(cdio) ;
        printf("\nAudio CD track count: %u\n", audio_cd_track_count);

        cdtext_t* cd_text_data = cdio_get_cdtext(cdio);
        const bool has_cd_text_data = cd_text_data != nullptr;

        printf("Has CD-Text data: %s\n", has_cd_text_data ? "yes" : "no");

        if(audio_cd_track_count > 0)
        {
            puts("");

            const track_t first_track_num{cdio_get_first_track_num(cdio)};

            uint32_t cd_audio_total_secs{};

            msf_t cd_audio_track_msf{};
            cdio_get_track_msf(cdio, first_track_num, &cd_audio_track_msf);
            uint32_t previous_track_seconds{cdio_audio_get_msf_seconds(&cd_audio_track_msf)};

            if(has_cd_text_data)
            {
                for(uint8_t field_index = MIN_CDTEXT_FIELD; field_index < MAX_CDTEXT_FIELDS; field_index++)
                {
                    const char * field_value = cdtext_get_const(cd_text_data, static_cast<cdtext_field_t>(field_index), 0);
                    if(field_value != nullptr)
                    {
                        std::string field_name{cdtext_field2str(static_cast<cdtext_field_t>(field_index))};
                        if(field_name.size() > 1)
                        {
                            std::transform(field_name.begin(), field_name.end(), field_name.begin(), ::tolower);
                        }
                        printf("Album %s: %s\n", field_name.c_str(), field_value);
                    }
                }

                puts("");
            }

            for(track_t track_index = first_track_num + 1; track_index <= audio_cd_track_count + 1; track_index++)
            {
                cdio_get_track_msf(cdio, track_index, &cd_audio_track_msf);
                const uint32_t track_offset_seconds = cdio_audio_get_msf_seconds(&cd_audio_track_msf);
                const uint32_t track_length = track_offset_seconds - previous_track_seconds;
                cd_audio_total_secs += track_length;

                printf("Track index: %u\n", track_index - 1);
                print_formatted_secs("Track start", previous_track_seconds);
                print_formatted_secs("Track end", track_offset_seconds);
                print_formatted_secs("Track length", track_length);

                if(has_cd_text_data)
                {
                    for(uint8_t field_index = MIN_CDTEXT_FIELD; field_index < MAX_CDTEXT_FIELDS; field_index++)
                    {
                        const char * field_value = cdtext_get_const(cd_text_data, static_cast<cdtext_field_t>(field_index), track_index);
                        if(field_value != nullptr)
                        {
                            std::string field_name{cdtext_field2str(static_cast<cdtext_field_t>(field_index))};
                            if(field_name.size() > 1)
                            {
                                std::transform(field_name.begin() + 1, field_name.end(), field_name.begin() + 1, ::tolower);
                            }

                            printf("%s: %s\n", field_name.c_str(), field_value);
                        }
                    }
                }

                puts("");

                previous_track_seconds = track_offset_seconds;
            }

            print_formatted_secs("Audio CD total time", cd_audio_total_secs);
        }

        cdio_subchannel_t subchannel_info{};
        if(cdio_audio_read_subchannel(cdio, &subchannel_info) == driver_return_code_t::DRIVER_OP_SUCCESS)
        {
            puts("");
            print_cdio_subchannel(subchannel_info);
        }
        else
        {
            printf("[Error] Failed to obtain disc sub-channel info\n");
        }
    }


}
