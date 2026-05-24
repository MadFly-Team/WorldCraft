#include <UI/WorldNameDialog.h>
#include <algorithm>
#include <cctype>

namespace UI
{

WorldNameDialog::WorldNameDialog()
	: m_isOpen(false)
	, m_worldName{ 0 }
{
}

bool WorldNameDialog::render()
{
	if (!m_isOpen) return false;

	ImGui::SetNextWindowSize(ImVec2(400, 180), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
							ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	if (!ImGui::Begin("Create New World", &m_isOpen, 
					  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
	{
		ImGui::End();
		return m_isOpen;
	}

	ImGui::Text("Enter a name for your new world:");
	ImGui::Spacing();

	// Auto-focus input field when dialog opens
	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
	}

	bool enterPressed = ImGui::InputText("##worldname", m_worldName, sizeof(m_worldName), 
										 ImGuiInputTextFlags_EnterReturnsTrue);

	// Show error message if validation failed
	if (!m_errorMessage.empty())
	{
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_errorMessage.c_str());
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Buttons
	bool createPressed = ImGui::Button("Create", ImVec2(120, 0));
	ImGui::SameLine();
	bool cancelPressed = ImGui::Button("Cancel", ImVec2(120, 0));

	// Handle create action
	if (createPressed || enterPressed)
	{
		std::string name(m_worldName);

		if (validateWorldName(name))
		{
			if (m_onConfirm)
			{
				m_onConfirm(name);
			}
			hide();
		}
	}

	// Handle cancel
	if (cancelPressed)
	{
		hide();
	}

	ImGui::End();
	return m_isOpen;
}

bool WorldNameDialog::validateWorldName(const std::string& name)
{
	// Check if name is empty
	if (name.empty())
	{
		m_errorMessage = "World name cannot be empty";
		return false;
	}

	// Check if name is too long
	if (name.length() > 32)
	{
		m_errorMessage = "World name is too long (max 32 characters)";
		return false;
	}

	// Check for invalid characters
	for (char c : name)
	{
		if (c == '/' || c == '\\' || c == ':' || c == '*' || 
			c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
		{
			m_errorMessage = "World name contains invalid characters";
			return false;
		}
	}

	// Check if name already exists
	for (const auto& existingName : m_existingNames)
	{
		if (existingName == name)
		{
			m_errorMessage = "A world with this name already exists";
			return false;
		}
	}

	m_errorMessage.clear();
	return true;
}

void WorldNameDialog::show(const std::string& suggestedName)
{
	m_isOpen = true;
	m_errorMessage.clear();

	if (!suggestedName.empty())
	{
		strncpy_s(m_worldName, sizeof(m_worldName), suggestedName.c_str(), _TRUNCATE);
	}
	else
	{
		m_worldName[0] = '\0';
	}
}

} // namespace UI
