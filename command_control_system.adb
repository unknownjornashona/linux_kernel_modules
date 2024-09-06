with Ada.Text_IO; use Ada.Text_IO;
with Ada.Integer_Text_IO;
with Ada.Exceptions; 
with Ada.Calendar;

procedure Command_Control_System is

    -- 日志记录过程
    procedure Log(Message: String) is
        Log_File : Ada.Text_IO.File_Type;
        Current_Time : Ada.Calendar.Time;
    begin
        -- 打开日志文件
        if not Ada.Text_IO.Is_Open(Log_File) then
            Ada.Text_IO.Open(Log_File, Ada.Text_IO.Out_File, "log.txt");
        end if;
        Current_Time := Ada.Calendar.Clock;
        Ada.Text_IO.Put_Line(Log_File, Ada.Calendar.Image(Current_Time) & " - " & Message);
    exception
        when others =>
            Put_Line("Error writing to log: " & Ada.Exceptions.Exception_Information);
    end Log;

    -- 自定义异常
    Bad_Command : exception;

    -- 任务类型定义
    task type Command_Task is
        entry Execute_Command(Input: String);
    end Command_Task;

    task body Command_Task is
    begin
        loop
            accept Execute_Command(Input: String) do
                -- 模拟命令执行
                if Input = "" then
                    raise Bad_Command;  -- 抛出异常
                else
                    Log("Executing command: " & Input);
                    delay 2.0; -- 模拟任务执行时间
                end if;
            end Execute_Command;
        end loop;
    end Command_Task;

    -- 任务类型定义
    task type Communication_Task is
    end Communication_Task;

    task body Communication_Task is
    begin
        loop
            Log("Handling Communication Task...");
            -- 处理通信逻辑
            delay 3.0; -- 模拟通信处理时间
        end loop;
    end Communication_Task;

    -- 其他系统组件和功能定义
    procedure Initialize_System is
    begin
        Log("Initializing Command Control System...");
    end Initialize_System;

begin
    Initialize_System; -- 初始化系统
    declare
        Command_Task_Instance : Command_Task;
        Communication_Task_Instance : Communication_Task;
        User_Input : String;
    begin
        -- 系统核心循环
        loop
            Put_Line("Enter Command (or 'exit' to stop): ");
            Get_Line(User_Input);
            if User_Input = "exit" then
                exit; -- 退出循环
            end if;
            Command_Task_Instance.Execute_Command(User_Input);
        end loop;
    end declare;

    Log("Shutting down Command Control System...");
end Command_Control_System;
