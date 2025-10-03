-- SPDX-FileCopyrightText: 2017 Daniel Ratcliffe
--
-- SPDX-License-Identifier: LicenseRef-CCPL

--[[- A simple menu bar.

> [!DANGER]
> This is an internal module and SHOULD NOT be used in your own code. It may
> be removed or changed at any time.

This provides a shared implementation of the menu bar used by the `edit` and
`paint` programs. This draws a menu bar at the bottom of the string, with a list
of options.

@local
]]


--[[- Create a new menu bar.

This should be called every time the menu is displayed.

@tparam { string... } items The menu items to display.
@return The menu.
]]
local function create(items)
    return {
        items = items,
        selected = 1,
    }
end

--[[- Draw the menu bar at the bottom of the screen.

This should be called when first displaying the menu, and if the whole screen is
redrawn (e.g. after a [`term_resize`]).

@param menu The menu bar to draw.
]]
local function draw(menu)

    local _, height = term.getSize()
    term.setCursorPos(1, height)

    term.clearLine()

    local active_color = colors.yellow
    term.setTextColor(colors.white)
    for k, v in pairs(menu.items) do
        if menu.selected == k then
            term.setTextColor(active_color)
            term.write("[")
            term.setTextColor(colors.white)
            term.write(v)
            term.setTextColor(active_color)
            term.write("]")
            term.setTextColor(colors.white)
        else
            term.write(" " .. v .. " ")
        end
    end
end

--[[- Process an event.

@param menu The menu bar to update.
@tparam string The event name.
@param ... Additional arguments to the event.
@treturn nil|boolean|string Either:

  - If no action was taken, return `nil`.
  - If the menu was closed, return `false`.
  - If an item was selected, return the item as a string.
]]
local function handle_event(menu, key)
		if key == keys.right then
				-- Move right
				menu.selected = menu.selected + 1
				if menu.selected > #menu.items then menu.selected = 1 end
				draw(menu)
		elseif key == keys.left and menu.selected > 1 then
				-- Move left
				menu.selected = menu.selected - 1
				if menu.selected < 1 then menu.selected = #menu.items end
				draw(menu)
		elseif key == keys.enter or key == keys.numPadEnter then
				-- Select an option
				return menu.items[menu.selected]
		elseif key == keys.control or keys == keys.alt then
				-- Cancel the menu
				return false
    elseif key:lower() >= "a" and key:lower() <= "z" then
			-- Select menu items
			local char = key:lower()
			for _, item in pairs(menu.items) do
					if item:sub(1, 1):lower() == char then return item end
			end
    end

    return nil
end


return { create = create, draw = draw, handle_event = handle_event }
