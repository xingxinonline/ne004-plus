# Display Demo debug init - POST DSP LOAD (Part 2)

# Release DSP reset
set {unsigned int}0x4000a018 = 1

echo \n>>> DSP Images loaded. DSP Released. Starting...\n
continue
