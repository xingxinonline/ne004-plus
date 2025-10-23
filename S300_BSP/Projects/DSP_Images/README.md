# DSP boot images

Place the DSP boot binary images in this folder. Expected filenames and addresses:

- dsp_dtcm_boot.bin  -> load address 0x44800000
- dsp_ptcm_boot.bin  -> load address 0x44A00000
- dsp_sram0_boot.bin -> load address 0x44000000

Optional additional images (if available in your SDK):

- dsp_psram_boot.bin
- dsp_sram1_boot.bin

Notes:

- These binaries were previously under S300_BSP/docs. They are now grouped here for clarity.
- The GDB init script gdbinit.display.dsp.gdb is configured to restore from this directory.
- If you still keep copies under docs/, you may remove them or keep as backup. Ensure the files here are the ones you want to load.

## Distribution and upload policy

Recommended options to provide these binaries to developers/CI:

1) Preferred: Git LFS

- Track DSP images with Git LFS to keep repo size reasonable while allowing "git clone" to fetch the correct versions.
- Example .gitattributes entries (already added at repo root):
	- `S300_BSP/Projects/DSP_Images/*.bin filter=lfs diff=lfs merge=lfs -text`
- Pros: versioned, reproducible; Cons: contributors/CI must enable Git LFS.

2) Alternative: Compressed archive + manual unpack

- Put a compressed bundle (zip/tar.gz) in your release artifacts or internal file server, e.g. `dsp_images_vYYYYMMDD.tar.gz`.
- Unpack to this folder so files match the expected names above.
- Provide checksums (sha256) and a short note in the release page or internal wiki.

3) Private/internal distribution

- Host binaries in a private storage (e.g., MinIO/S3/artifact registry). Provide a short fetch script to download to this directory.
- Keep credentials outside the repo, use environment variables for CI.

For the Display Demo `dbg_display_dsp` target to work out-of-the-box, this directory must contain at least these files:

- `dsp_dtcm_boot.bin`
- `dsp_ptcm_boot.bin`
- `dsp_sram0_boot.bin`

Quick LFS usage for contributors:

```bash
# One-time per machine
git lfs install

# After adding or updating DSP images
git add S300_BSP/Projects/DSP_Images/*.bin
git commit -m "Add/update DSP boot images (LFS)"
```

If you cannot use LFS, we suggest attaching a compressed bundle to your release and referencing it from the top-level README.
