for /r core %%f in (*.cpp *.c *.h *.hpp *.cc) do (
    clang-format -i "%%f"
)
