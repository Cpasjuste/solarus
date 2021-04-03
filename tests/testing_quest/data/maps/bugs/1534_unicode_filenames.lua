local map = ...
local game = map:get_game()

function map:on_started()
  -- Define patterns to test.
  local patterns = {
    "ΣΑΠΦΩ", "ñandú", "Максмраз"
  }

  -- Iterate over the patterns and test file operations.
  for _, pattern in ipairs(patterns) do
    local path = "1534_" .. pattern

    assert(not sol.file.exists(path), "path already exists: " .. path)

    local f, err = sol.file.open(path, "w")
    assert(f, "could not open path '" .. path .. "' for writing: " .. (err or ""))
    f:write("hello")
    f:close()

    assert(sol.file.exists(path), "path does not exist after writing: " .. path)
    assert(not sol.file.is_dir(path), "path should not be a directory: " .. path)
    assert(not sol.file.list_dir(path), "path should not have directory list: " .. path)

    local f, err = sol.file.open(path)
    assert(f, "could not open path '" .. path .. "' for reading: " .. (err or ""))
    assert_equal(f:read("*a"), "hello")
    f:close()

    sol.file.remove(path)
    assert(not sol.file.exists(path), "path still exists after removing: " .. path)

    sol.file.mkdir(path)
    assert(sol.file.is_dir(path), "path should be a directory: " .. path)
    assert(sol.file.list_dir(path), "path should have directory list: " .. path)

    sol.file.remove(path)
    assert(not sol.file.exists(path), "path still exists after removing: " .. path)
  end

  -- We are done.
  sol.main.exit()
end
